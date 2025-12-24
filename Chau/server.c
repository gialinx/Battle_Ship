// server with database integration - ELO rating system
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "src/database.h"

#define PORT 5500
#define MAX_PLAYER 2
#define BUFF_SIZE 8192
#define MAP_SIZE 13
#define MAX_SHIP 4

// ==========================
// CẤU TRÚC TÀU
// ==========================
typedef struct {
    int length;
    int x[MAP_SIZE];
    int y[MAP_SIZE];
    int hits;
    int alive;
} Ship;

// ==========================
// CẤU TRÚC NGƯỜI CHƠI (đã thêm user info)
// ==========================
typedef struct {
    int fd;
    char map[MAP_SIZE][MAP_SIZE];
    char enemy_map[MAP_SIZE][MAP_SIZE];
    Ship ships[MAX_SHIP];
    int ship_count;
    int ready;
    
    // Database integration
    int user_id;              // ID từ database (-1 = chưa đăng nhập)
    char username[50];        // Tên đăng nhập
    int is_authenticated;     // Đã đăng nhập chưa
} Player;

// ==========================
// CẤU TRÚC TRẬN ĐẤU (đã thêm stats)
// ==========================
typedef struct {
    Player players[MAX_PLAYER];
    int player_count;
    int current_turn;
    pthread_mutex_t lock;
    
    // Game stats cho ELO
    int player_shots[MAX_PLAYER];     // Tổng số lần bắn
    int player_hits[MAX_PLAYER];      // Số lần bắn trúng
    char moves_log[4096];             // Log các nước đi
    int match_started;                // Trận đã bắt đầu chưa
} Match;

Match match;

// ==========================
// KHỞI TẠO BẢN ĐỒ RỖNG
// ==========================
void init_map(char map[MAP_SIZE][MAP_SIZE]){
    for(int i=0;i<MAP_SIZE;i++)
        for(int j=0;j<MAP_SIZE;j++)
            map[i][j]='-';
}

// ==========================
// KIỂM TRA NGƯỜI CHƠI ĐÃ ĐẶT ĐỦ TÀU CHƯA
// ==========================
int check_ready(Player *p){
    int count4=0,count3=0,count2=0;
    for(int i=0;i<p->ship_count;i++){
        if(p->ships[i].length==4) count4++;
        else if(p->ships[i].length==3) count3++;
        else if(p->ships[i].length==2) count2++;
    }
    return (count4>=1 && count3>=1 && count2>=2);
}

// ==========================
// ĐÁNH DẤU TÀU ĐÃ CHÌM
// ==========================
void mark_sunk(Player *p, Player *enemy){
    for(int s=0;s<p->ship_count;s++){
        Ship *sh=&p->ships[s];
        if(sh->alive==0){
            for(int k=0;k<sh->length;k++){
                p->map[sh->y[k]][sh->x[k]]='@';
                enemy->enemy_map[sh->y[k]][sh->x[k]]='@';
            }
        }
    }
}

// ==========================
// GỬI TRẠNG THÁI MAP
// ==========================
void send_state(Player *p){
    char msg[BUFF_SIZE];
    msg[0]='\0';

    strcat(msg,"STATE:\nOWN MAP\n");
    for(int i=0;i<MAP_SIZE;i++){
        for(int j=0;j<MAP_SIZE;j++){
            char tmp[2]; tmp[0]=p->map[i][j]; tmp[1]='\0';
            strcat(msg,tmp); strcat(msg," ");
        }
        strcat(msg,"\n");
    }

    strcat(msg,"ENEMY MAP\n");
    for(int i=0;i<MAP_SIZE;i++){
        for(int j=0;j<MAP_SIZE;j++){
            char tmp[2]; tmp[0]=p->enemy_map[i][j]; tmp[1]='\0';
            strcat(msg,tmp); strcat(msg," ");
        }
        strcat(msg,"\n");
    }

    strcat(msg,"#");
    send(p->fd, msg, strlen(msg), 0);
}

// ==========================
// GỬI THÔNG BÁO CHO CẢ 2 CLIENT
// ==========================
void broadcast_nolock(const char *msg){
    for(int i=0;i<match.player_count;i++){
        send(match.players[i].fd, msg, strlen(msg), 0);
    }
}

void broadcast(const char *msg){
    pthread_mutex_lock(&match.lock);
    broadcast_nolock(msg);
    pthread_mutex_unlock(&match.lock);
}

// ==========================
// XỬ LÝ ĐĂNG KÝ
// ==========================
void handle_register(Player *p, char *buffer){
    char username[50], password[50];
    if(sscanf(buffer+9, "%[^:]:%s", username, password) == 2){
        int user_id = db_register_user(username, password);
        if(user_id > 0){
            send(p->fd, "REGISTER_OK#", 12, 0);
            printf("User registered: %s (ID: %d)\n", username, user_id);
        } else {
            send(p->fd, "REGISTER_FAIL:Username exists#", 31, 0);
        }
    } else {
        send(p->fd, "REGISTER_FAIL:Invalid format#", 30, 0);
    }
}

// ==========================
// XỬ LÝ ĐĂNG NHẬP
// ==========================
void handle_login(Player *p, char *buffer){
    char username[50], password[50];
    if(sscanf(buffer+6, "%[^:]:%s", username, password) == 2){
        UserProfile profile;
        int user_id = db_login_user(username, password, &profile);
        
        if(user_id > 0){
            pthread_mutex_lock(&match.lock);
            p->user_id = user_id;
            strcpy(p->username, profile.username);
            p->is_authenticated = 1;
            pthread_mutex_unlock(&match.lock);
            
            char response[BUFF_SIZE];
            snprintf(response, sizeof(response), 
                    "LOGIN_OK:%s:%d:%d:%d:%d#", 
                    profile.username, profile.total_games, 
                    profile.wins, profile.elo_rating, user_id);
            send(p->fd, response, strlen(response), 0);
            printf("User logged in: %s (ELO: %d)\n", username, profile.elo_rating);
        } else {
            send(p->fd, "LOGIN_FAIL:Invalid credentials#", 32, 0);
        }
    }
}

// ==========================
// XỬ LÝ LOGOUT
// ==========================
void handle_logout(Player *p){
    if(p->user_id > 0){
        db_logout_user(p->user_id);
        printf("User logged out: %s\n", p->username);
    }
    pthread_mutex_lock(&match.lock);
    p->user_id = -1;
    p->is_authenticated = 0;
    pthread_mutex_unlock(&match.lock);
    send(p->fd, "LOGOUT_OK#", 10, 0);
}

// ==========================
// XỬ LÝ XEM PROFILE
// ==========================
void handle_profile(Player *p){
    if(p->user_id <= 0){
        send(p->fd, "PROFILE_FAIL:Not logged in#", 28, 0);
        return;
    }
    
    UserProfile profile;
    if(db_get_user_profile(p->user_id, &profile) == 0){
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response), 
                "PROFILE:%s:%d:%d:%d:%d:%d#",
                profile.username, profile.total_games, profile.wins, 
                profile.losses, profile.total_score, profile.elo_rating);
        send(p->fd, response, strlen(response), 0);
    }
}

// ==========================
// XỬ LÝ XEM LỊCH SỬ
// ==========================
void handle_history(Player *p){
    if(p->user_id <= 0){
        send(p->fd, "HISTORY_FAIL:Not logged in#", 28, 0);
        return;
    }
    
    MatchHistory* matches;
    int count;
    
    if(db_get_match_history(p->user_id, &matches, &count) == 0){
        char response[BUFF_SIZE * 2];
        int offset = sprintf(response, "HISTORY:%d#", count);
        
        for(int i=0; i<count && i<10; i++){
            offset += snprintf(response + offset, sizeof(response) - offset,
                "M%d:W%d:ELO%+d#", 
                matches[i].match_id,
                matches[i].winner_id,
                (matches[i].player1_id == p->user_id) ? 
                    matches[i].player1_elo_gain : matches[i].player2_elo_gain
            );
        }
        
        send(p->fd, response, strlen(response), 0);
        free(matches);
    }
}

// ==========================
// XỬ LÝ FIRE VỚI STATS
// ==========================
void process_fire(Player *attacker, Player *target, int x, int y, int attacker_id){
    char cell = target->map[y][x];
    
    // Ghi log
    char log_entry[64];
    snprintf(log_entry, sizeof(log_entry), "P%d:%d,%d:", attacker_id, x, y);
    strcat(match.moves_log, log_entry);

    match.player_shots[attacker_id]++;

    // MISS
    if(cell=='-' || cell=='x' || cell=='o' || cell=='@'){
        attacker->enemy_map[y][x]='x';
        strcat(match.moves_log, "MISS;");
        
        char res[64]; 
        snprintf(res,sizeof(res),"RESULT:MISS,%d,%d#",x+1,y+1);
        send(attacker->fd,res,strlen(res),0);
        printf("DEBUG: MISS at (%d,%d)\n", x+1, y+1);
    }
    // HIT
    else if(cell>='2' && cell<='5'){
        attacker->enemy_map[y][x]='o';
        target->map[y][x]='o';
        match.player_hits[attacker_id]++;
        strcat(match.moves_log, "HIT;");

        for(int s=0;s<target->ship_count;s++){
            Ship *sh=&target->ships[s];
            for(int k=0;k<sh->length;k++){
                if(sh->x[k]==x && sh->y[k]==y){
                    sh->hits++;
                    if(sh->hits==sh->length){
                        sh->alive=0;
                        printf("DEBUG: Ship length %d sunk\n", sh->length);
                    }
                    break;
                }
            }
        }

        mark_sunk(target, attacker);

        char res[64]; 
        snprintf(res,sizeof(res),"RESULT:HIT,%d,%d#",x+1,y+1);
        send(attacker->fd,res,strlen(res),0);
        printf("DEBUG: HIT at (%d,%d)\n", x+1, y+1);
    }
}

// ==========================
// LƯU KẾT QUẢ TRẬN ĐẤU VÀO DATABASE
// ==========================
void save_match_result(int winner_id){
    // Chỉ lưu nếu cả 2 đã đăng nhập
    if(match.players[0].user_id <= 0 || match.players[1].user_id <= 0){
        printf("DEBUG: Match not saved - players not authenticated\n");
        return;
    }

    MatchHistory m;
    m.player1_id = match.players[0].user_id;
    m.player2_id = match.players[1].user_id;
    m.winner_id = winner_id;
    
    // Tính điểm
    m.player1_score = match.player_hits[0] * 10;
    m.player2_score = match.player_hits[1] * 10;
    
    // Tính hit_diff
    m.player1_hit_diff = match.player_hits[0] - match.player_hits[1];
    m.player2_hit_diff = match.player_hits[1] - match.player_hits[0];
    
    // Tính accuracy
    m.player1_accuracy = (match.player_shots[0] > 0) ? 
        (float)match.player_hits[0] / match.player_shots[0] : 0.0;
    m.player2_accuracy = (match.player_shots[1] > 0) ? 
        (float)match.player_hits[1] / match.player_shots[1] : 0.0;
    
    // Copy moves log
    strncpy(m.match_data, match.moves_log, sizeof(m.match_data)-1);
    m.match_data[sizeof(m.match_data)-1] = '\0';
    
    // Lưu match (tự động tính ELO)
    int match_id = db_save_match(&m);
    
    // Cập nhật stats
    db_update_score(m.player1_id, m.player1_score, winner_id == m.player1_id);
    db_update_score(m.player2_id, m.player2_score, winner_id == m.player2_id);
    
    printf("Match saved: ID=%d, Winner=%d, P1_ELO:%+d, P2_ELO:%+d\n",
           match_id, winner_id, m.player1_elo_gain, m.player2_elo_gain);
    
    // Gửi thông tin ELO về client
    char elo_msg[256];
    snprintf(elo_msg, sizeof(elo_msg), 
            "ELO_UPDATE:P1:%+d:P2:%+d#",
            m.player1_elo_gain, m.player2_elo_gain);
    broadcast(elo_msg);
}

// ==========================
// THREAD XỬ LÝ CLIENT
// ==========================
void *client_handler(void *arg){
    int id = *(int*)arg; free(arg);
    Player *p = &match.players[id];
    char buff[BUFF_SIZE];

    printf("DEBUG: handler started for player %d\n", id);

    while(1){
        int n = recv(p->fd, buff, BUFF_SIZE-1, 0);
        if(n <= 0){
            printf("Player %d disconnected\n", id);
            if(p->user_id > 0) db_logout_user(p->user_id);
            break;
        }
        buff[n] = '\0';

        char *tok_save = NULL;
        char *cmd = strtok_r(buff, "#", &tok_save);

        while(cmd){
            while(*cmd=='\n' || *cmd=='\r') cmd++;

            // ==========================
            // DATABASE COMMANDS
            // ==========================
            if(strncmp(cmd,"REGISTER:",9)==0){
                handle_register(p, cmd);
            }
            else if(strncmp(cmd,"LOGIN:",6)==0){
                handle_login(p, cmd);
            }
            else if(strcmp(cmd,"LOGOUT")==0){
                handle_logout(p);
            }
            else if(strcmp(cmd,"PROFILE")==0){
                handle_profile(p);
            }
            else if(strcmp(cmd,"HISTORY")==0){
                handle_history(p);
            }

            // ==========================
            // GAME COMMANDS
            // ==========================
            else if(strncmp(cmd,"PLACE:",6)==0){
                int length,x,y; char dir;
                sscanf(cmd+6,"%d,%d,%d,%c",&length,&x,&y,&dir);
                printf("DEBUG: Player %d PLACE len=%d x=%d y=%d dir=%c\n",
                       id,length,x,y,dir);

                x--; y--;

                if(dir>='a' && dir<='z') dir = dir - 'a' + 'A';
                if(dir!='H' && dir!='V'){
                    send(p->fd,"ERROR:Invalid direction#",24,0);
                    cmd = strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                int max_ships = (length==4||length==3)?1:2;
                int count=0;
                for(int i=0;i<p->ship_count;i++)
                    if(p->ships[i].length==length) count++;
                if(count>=max_ships){
                    send(p->fd,"ERROR:Already placed all ships of this type#",45,0);
                    cmd = strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                int dx = (dir=='H')?1:0;
                int dy = (dir=='V')?1:0;
                int ex = x + dx*(length-1);
                int ey = y + dy*(length-1);

                if(x<0 || y<0 || ex<0 || ey<0 ||
                   x>=MAP_SIZE || y>=MAP_SIZE ||
                   ex>=MAP_SIZE || ey>=MAP_SIZE){
                    send(p->fd,"ERROR:Out of bounds#",20,0);
                    cmd=strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                int overlap=0;
                for(int i=0;i<length;i++){
                    int tx=x+dx*i;
                    int ty=y+dy*i;
                    if(p->map[ty][tx] != '-') { overlap=1; break; }
                }
                if(overlap){
                    send(p->fd,"ERROR:Ship overlaps#",20,0);
                    cmd=strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                pthread_mutex_lock(&match.lock);

                Ship *sh=&p->ships[p->ship_count];
                sh->length=length;
                sh->hits=0;
                sh->alive=1;

                for(int i=0;i<length;i++){
                    int tx=x+dx*i;
                    int ty=y+dy*i;
                    p->map[ty][tx] = '0'+length;
                    sh->x[i]=tx;
                    sh->y[i]=ty;
                }

                p->ship_count++;
                pthread_mutex_unlock(&match.lock);

                send(p->fd,"PLACE_OK:#",10,0);
                send_state(p);
            }

            else if(strcmp(cmd,"READY")==0){
                printf("DEBUG: Player %d sent READY\n", id);

                pthread_mutex_lock(&match.lock);

                if(check_ready(p)){
                    p->ready=1;
                    send(p->fd,"READY_OK:#",10,0);

                    int all_ready=1;
                    for(int i=0;i<MAX_PLAYER;i++){
                        if(match.players[i].ready==0) all_ready=0;
                    }

                    if(all_ready && match.player_count==MAX_PLAYER){
                        printf("DEBUG: MATCH START\n");
                        
                        // Reset stats
                        match.player_shots[0] = 0;
                        match.player_shots[1] = 0;
                        match.player_hits[0] = 0;
                        match.player_hits[1] = 0;
                        memset(match.moves_log, 0, sizeof(match.moves_log));
                        match.match_started = 1;
                        
                        usleep(100000);
                        broadcast_nolock("MATCH_START:#");

                        for(int i=0;i<MAX_PLAYER;i++)
                            send_state(&match.players[i]);

                        match.current_turn=0;
                        send(match.players[0].fd,"YOUR_TURN:#",11,0);
                        send(match.players[1].fd,"WAIT_YOUR_TURN:#",16,0);
                    }
                    else {
                        send(p->fd,"Waiting for opponent#",21,0);
                    }
                }
                else {
                    send(p->fd,"ERROR:Place all ships first#",29,0);
                }

                pthread_mutex_unlock(&match.lock);
            }

            else if(strncmp(cmd,"FIRE:",5)==0){
                int x,y;
                sscanf(cmd+5,"%d,%d",&x,&y);
                printf("DEBUG: Player %d FIRE to (%d,%d)\n",id,x,y);

                x--; y--;

                if(x<0 || y<0 || x>=MAP_SIZE || y>=MAP_SIZE){
                    send(p->fd,"ERROR:Out of bounds#",20,0);
                    cmd=strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                pthread_mutex_lock(&match.lock);

                if(id != match.current_turn){
                    send(p->fd,"WAIT_YOUR_TURN:#",16,0);
                    pthread_mutex_unlock(&match.lock);
                }
                else {
                    char already = match.players[id].enemy_map[y][x];
                    if(already=='x' || already=='o' || already=='@'){
                        send(p->fd,"ERROR:Already fired#",20,0);
                        pthread_mutex_unlock(&match.lock);
                    }
                    else{
                        process_fire(&match.players[id], &match.players[1-id], x, y, id);

                        int opp_alive=0;
                        for(int s=0;s<match.players[1-id].ship_count;s++){
                            if(match.players[1-id].ships[s].alive){
                                opp_alive=1;
                                break;
                            }
                        }

                        if(!opp_alive){
                            int winner_id = match.players[id].user_id;
                            send(match.players[id].fd,"YOU WIN:#",9,0);
                            send(match.players[1-id].fd,"YOU LOSE:#",10,0);
                            send_state(&match.players[0]);
                            send_state(&match.players[1]);
                            
                            // Lưu kết quả vào database
                            save_match_result(winner_id);
                            
                            pthread_mutex_unlock(&match.lock);
                        }
                        else{
                            if(match.players[id].enemy_map[y][x]=='o'){
                                send(match.players[id].fd,"HIT_CONTINUE:#",14,0);
                                send(match.players[1-id].fd,"OPPONENT_HIT_CONTINUE:#",23,0);
                            }
                            else{
                                match.current_turn = 1-id;
                                send(match.players[match.current_turn].fd,"YOUR_TURN:#",11,0);
                                send(match.players[1-match.current_turn].fd,"WAIT_YOUR_TURN:#",16,0);
                            }

                            send_state(&match.players[0]);
                            send_state(&match.players[1]);
                            pthread_mutex_unlock(&match.lock);
                        }
                    }
                }
            }

            else if(strcmp(cmd,"QUIT")==0){
                printf("DEBUG: Player %d QUIT\n", id);
                if(p->user_id > 0) db_logout_user(p->user_id);
                close(p->fd);
                return NULL;
            }

            else {
                send(p->fd,"ERROR:Unknown command#",22,0);
            }

            cmd = strtok_r(NULL, "#", &tok_save);
        }
    }

    close(p->fd);
    return NULL;
}

// ==========================
// MAIN
// ==========================
int main(){
    // Khởi tạo database
    if(db_init() != 0){
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    printf("Database initialized successfully\n");

    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    match.player_count = 0;
    match.current_turn = 0;
    match.match_started = 0;
    pthread_mutex_init(&match.lock, NULL);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){ perror("socket"); exit(1); }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        perror("bind"); exit(1);
    }
    listen(listenfd, 10);
    printf("Server listening at 127.0.0.1:%d\n", PORT);

    while(match.player_count < MAX_PLAYER){
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);

        pthread_mutex_lock(&match.lock);
        int id = match.player_count;

        match.players[id].fd = connfd;
        match.players[id].user_id = -1;
        match.players[id].is_authenticated = 0;
        strcpy(match.players[id].username, "Guest");
        
        init_map(match.players[id].map);
        init_map(match.players[id].enemy_map);
        match.players[id].ship_count = 0;
        match.players[id].ready = 0;

        match.player_count++;
        pthread_mutex_unlock(&match.lock);

        int *arg = malloc(sizeof(int));
        *arg = id;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, arg);
        pthread_detach(tid);

        printf("Player %d connected.\n", id+1);
    }

    while(1) sleep(1);

    db_close();
    close(listenfd);
    return 0;
}
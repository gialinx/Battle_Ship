// server_debug.c -- improved READY handling + logging + explicit READY_OK
// BẢN ĐÃ ĐƯỢC COMMENT CHI TIẾT TỪNG PHẦN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define PORT 5500
#define MAX_PLAYER 2
#define BUFF_SIZE 8192
#define MAP_SIZE 13
#define MAX_SHIP 4

// ==========================
// CẤU TRÚC TÀU
// ==========================
typedef struct {
    int length;                     // độ dài tàu (2,3,4)
    int x[MAP_SIZE];                // danh sách các điểm trên trục x
    int y[MAP_SIZE];                // danh sách các điểm trên trục y
    int hits;                       // số lượng ô đã bị bắn trúng
    int alive;                      // còn sống? 1 = còn, 0 = chìm
} Ship;

// ==========================
// CẤU TRÚC NGƯỜI CHƠI
// ==========================
typedef struct {
    int fd;                         // socket kết nối đến client
    char map[MAP_SIZE][MAP_SIZE];   // bản đồ thật của người chơi
    char enemy_map[MAP_SIZE][MAP_SIZE]; // bản đồ ghi nhận đối thủ (x, o, @)
    Ship ships[MAX_SHIP];           // danh sách tàu đã đặt
    int ship_count;                 // số tàu đã đặt
    int ready;                      // đã READY hay chưa
} Player;

// ==========================
// CẤU TRÚC TRẬN ĐẤU
// ==========================
typedef struct {
    Player players[MAX_PLAYER];     // 2 người chơi
    int player_count;               // số người đã kết nối (tối đa 2)
    int current_turn;               // lượt hiện tại (0 hoặc 1)
    pthread_mutex_t lock;           // khóa để tránh conflict
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
// 1 tàu độ dài 4
// 1 tàu độ dài 3
// 2 tàu độ dài 2
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
// ĐÁNH DẤU TÀU ĐÃ CHÌM → '@'
// Cập nhật cả map thật và map Enemy
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
// Bao gồm OWN MAP + ENEMY MAP
// Dùng khi:
//  - đặt tàu
//  - cập nhật sau FIRE
//  - bắt đầu trận
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
    int result = send(p->fd, msg, strlen(msg), MSG_DONTWAIT);
    if(result < 0) {
        printf("DEBUG: Failed to send STATE to player (non-blocking)\n");
    }
}

// ==========================
// GỬI THÔNG BÁO CHO CẢ 2 CLIENT (internal, no lock)
// ==========================
void broadcast_nolock(const char *msg){
    for(int i=0;i<match.player_count;i++){
        // Use blocking send - should be fast for small messages
        int result = send(match.players[i].fd, msg, strlen(msg), 0);
        if(result < 0) {
            printf("DEBUG: Failed to send '%s' to player %d (fd=%d): errno=%d\n", msg, i, match.players[i].fd, errno);
        } else {
            printf("DEBUG: Sent '%s' (%d bytes) to player %d (fd=%d)\n", msg, result, i, match.players[i].fd);
        }
    }
}

// ==========================
// GỬI THÔNG BÁO CHO CẢ 2 CLIENT (with lock)
// ==========================
void broadcast(const char *msg){
    pthread_mutex_lock(&match.lock);
    broadcast_nolock(msg);
    pthread_mutex_unlock(&match.lock);
}

// ==========================
// XỬ LÝ HÀNH ĐỘNG FIRE
// - MISS: 'x'
// - HIT:  'o'
// - SUNK: '@'
// ==========================
void process_fire(Player *attacker, Player *target, int x, int y){
    char cell = target->map[y][x];

    // MISS
    if(cell=='-' || cell=='x' || cell=='o' || cell=='@'){
        attacker->enemy_map[y][x]='x';
        char res[64]; snprintf(res,sizeof(res),"RESULT:MISS,%d,%d#",x+1,y+1);
        send(attacker->fd,res,strlen(res),0);
        printf("DEBUG: MISS at (%d,%d)\n", x+1, y+1);
    }
    // HIT
    else if(cell>='2' && cell<='5'){
        attacker->enemy_map[y][x]='o';
        target->map[y][x]='o';

        // kiểm tra có chìm tàu không
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

        char res[64]; snprintf(res,sizeof(res),"RESULT:HIT,%d,%d#",x+1,y+1);
        send(attacker->fd,res,strlen(res),0);
        printf("DEBUG: HIT at (%d,%d)\n", x+1, y+1);
    }
}

// ==========================
// THREAD XỬ LÝ TỪNG CLIENT
// Nhận lệnh PLACE / READY / FIRE / QUIT
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
            break;
        }
        buff[n] = '\0';

        // client có thể gửi nhiều lệnh cùng lúc, phân tách bằng '#'
        char *tok_save = NULL;
        char *cmd = strtok_r(buff, "#", &tok_save);

        while(cmd){
            // bỏ xuống dòng
            while(*cmd=='\n' || *cmd=='\r') cmd++;

            // ==========================
            // LỆNH ĐẶT TÀU PLACE
            // ==========================
            if(strncmp(cmd,"PLACE:",6)==0){
                int length,x,y; char dir;
                sscanf(cmd+6,"%d,%d,%d,%c",&length,&x,&y,&dir);
                printf("DEBUG: Player %d PLACE len=%d x=%d y=%d dir=%c\n",
                       id,length,x,y,dir);

                x--; y--;  // quy về 0-index

                // đổi hướng sang H/V
                if(dir>='a' && dir<='z') dir = dir - 'a' + 'A';
                if(dir!='H' && dir!='V'){
                    send(p->fd,"ERROR:Invalid direction (H/V).#",31,0);
                    cmd = strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                // giới hạn số lượng tàu từng loại
                int max_ships = (length==4||length==3)?1:2;
                int count=0;
                for(int i=0;i<p->ship_count;i++)
                    if(p->ships[i].length==length) count++;
                if(count>=max_ships){
                    send(p->fd,"ERROR:You have already placed all ships of this type.#",58,0);
                    cmd = strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                // tính vị trí cuối theo hướng H/V
                int dx = (dir=='H')?1:0;
                int dy = (dir=='V')?1:0;
                int ex = x + dx*(length-1);
                int ey = y + dy*(length-1);

                // kiểm tra out-of-bounds
                if(x<0 || y<0 || ex<0 || ey<0 ||
                   x>=MAP_SIZE || y>=MAP_SIZE ||
                   ex>=MAP_SIZE || ey>=MAP_SIZE){
                    send(p->fd,"ERROR:Ship placement out of bounds.#",36,0);
                    cmd=strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                // kiểm tra overlap
                int overlap=0;
                for(int i=0;i<length;i++){
                    int tx=x+dx*i;
                    int ty=y+dy*i;
                    if(p->map[ty][tx] != '-') { overlap=1; break; }
                }
                if(overlap){
                    send(p->fd,"ERROR:Ship overlaps existing ship.#",35,0);
                    cmd=strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                // ghi tàu vào map
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

            // ==========================
            // READY
            // ==========================
            else if(strcmp(cmd,"READY")==0){
                printf("DEBUG: Player %d sent READY\n", id);

                pthread_mutex_lock(&match.lock);

                if(check_ready(p)){
                    p->ready=1;
                    send(p->fd,"READY_OK:#",10,0);

                    // kiểm tra cả 2 READY
                    int all_ready=1;
                    for(int i=0;i<MAX_PLAYER;i++){
                        if(match.players[i].ready==0) all_ready=0;
                    }
                    
                    printf("DEBUG: all_ready=%d, player_count=%d, MAX_PLAYER=%d\n", 
                           all_ready, match.player_count, MAX_PLAYER);
                    printf("DEBUG: Player 0 ready=%d, Player 1 ready=%d\n",
                           match.players[0].ready, match.players[1].ready);

                    if(all_ready && match.player_count==MAX_PLAYER){
                        printf("DEBUG: MATCH START\n");
                        fflush(stdout);
                        
                        // Đợi một chút để client nhận READY_OK trước
                        usleep(100000); // 100ms
                        
                        printf("DEBUG: Broadcasting MATCH_START...\n");
                        fflush(stdout);
                        broadcast_nolock("MATCH_START:#");
                        printf("DEBUG: MATCH_START broadcasted\n");
                        fflush(stdout);

                        // gửi state trước
                        printf("DEBUG: Sending STATE to both players...\n");
                        fflush(stdout);
                        for(int i=0;i<MAX_PLAYER;i++)
                            send_state(&match.players[i]);
                        printf("DEBUG: STATE sent\n");
                        fflush(stdout);

                        match.current_turn=0;

                        // thông báo lượt
                        printf("DEBUG: Sending YOUR_TURN to player 0...\n");
                        fflush(stdout);
                        int r1 = send(match.players[0].fd,"YOUR_TURN:#",11,MSG_DONTWAIT);
                        printf("DEBUG: YOUR_TURN result: %d\n", r1);
                        fflush(stdout);
                        
                        printf("DEBUG: Sending WAIT_YOUR_TURN to player 1...\n");
                        fflush(stdout);
                        int r2 = send(match.players[1].fd,"WAIT_YOUR_TURN:#",17,MSG_DONTWAIT);
                        printf("DEBUG: WAIT_YOUR_TURN result: %d\n", r2);
                        fflush(stdout);
                    }
                    else {
                        send(p->fd,"Waiting for opponent.#",23,0);
                    }
                }
                else {
                    send(p->fd,"ERROR:Place all required ships before READY.#",44,0);
                }

                pthread_mutex_unlock(&match.lock);
            }

            // ==========================
            // FIRE
            // ==========================
            else if(strncmp(cmd,"FIRE:",5)==0){
                int x,y;
                sscanf(cmd+5,"%d,%d",&x,&y);
                printf("DEBUG: Player %d FIRE to (%d,%d)\n",id,x,y);

                x--; y--;

                if(x<0 || y<0 || x>=MAP_SIZE || y>=MAP_SIZE){
                    send(p->fd,"ERROR:Fire coordinates out of bounds.#",37,0);
                    cmd=strtok_r(NULL,"#",&tok_save);
                    continue;
                }

                pthread_mutex_lock(&match.lock);

                // nếu chưa đến lượt
                if(id != match.current_turn){
                    send(p->fd,"WAIT_YOUR_TURN:#",17,0);
                    pthread_mutex_unlock(&match.lock);
                }
                else {
                    // kiểm tra đã bắn ô này chưa
                    char already = match.players[id].enemy_map[y][x];
                    if(already=='x' || already=='o' || already=='@'){
                        send(p->fd,"ERROR:You already fired at this coordinate.#",44,0);
                        pthread_mutex_unlock(&match.lock);
                    }
                    else{
                        process_fire(&match.players[id], &match.players[1-id], x, y);

                        // kiểm tra đối thủ còn tàu không
                        int opp_alive=0;
                        for(int s=0;s<match.players[1-id].ship_count;s++){
                            if(match.players[1-id].ships[s].alive){
                                opp_alive=1;
                                break;
                            }
                        }

                        if(!opp_alive){
                            // thắng
                            send(match.players[id].fd,"YOU WIN:#",9,0);
                            send(match.players[1-id].fd,"YOU LOSE:#",10,0);
                            send_state(&match.players[0]);
                            send_state(&match.players[1]);
                            pthread_mutex_unlock(&match.lock);
                        }
                        else{
                            // hit = bắn tiếp
                            if(match.players[id].enemy_map[y][x]=='o'){
                                send(match.players[id].fd,"HIT_CONTINUE:#",15,0);
                                send(match.players[1-id].fd,"OPPONENT_HIT_CONTINUE:#",23,0);
                            }
                            // miss = đổi lượt
                            else{
                                match.current_turn = 1-id;
                                send(match.players[match.current_turn].fd,"YOUR_TURN:#",11,0);
                                send(match.players[1-match.current_turn].fd,"WAIT_YOUR_TURN:#",17,0);
                            }

                            // cập nhật map 2 bên
                            send_state(&match.players[0]);
                            send_state(&match.players[1]);
                            pthread_mutex_unlock(&match.lock);
                        }
                    }
                }
            }

            // ==========================
            // QUIT
            // ==========================
            else if(strcmp(cmd,"QUIT")==0){
                printf("DEBUG: Player %d QUIT\n", id);
                close(p->fd);
                return NULL;
            }

            // ==========================
            // LỆNH KHÔNG HỢP LỆ
            // ==========================
            else {
                send(p->fd,"ERROR:Unknown command.#",22,0);
            }

            cmd = strtok_r(NULL, "#", &tok_save);
        }
    }

    close(p->fd);
    return NULL;
}

// ==========================
// HÀM MAIN – SERVER
// ==========================
int main(){
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    match.player_count = 0;
    match.current_turn = 0;
    pthread_mutex_init(&match.lock, NULL);

    // tạo socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){ perror("socket"); exit(1); }

    // cấu hình server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        perror("bind"); exit(1);
    }
    listen(listenfd, 10);
    printf("Server listening at 127.0.0.1:%d\n", PORT);

    // chờ đủ 2 người chơi
    while(match.player_count < MAX_PLAYER){
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);

        pthread_mutex_lock(&match.lock);
        int id = match.player_count;

        match.players[id].fd = connfd;
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

    // server chạy vô hạn
    while(1) sleep(1);

    close(listenfd);
    return 0;
}

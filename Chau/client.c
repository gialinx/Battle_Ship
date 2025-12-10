// client_fixed.c
// Client improved: robust STATE parsing, reset maps, skip empty lines,
// render header aligned, show messages below maps.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 5500
#define BUFF_SIZE 8192
#define MAP_SIZE 13

// Socket kết nối server
int sockfd;

// 2 bản đồ client giữ (OWN MAP và ENEMY MAP)
char own_map[MAP_SIZE][MAP_SIZE];
char enemy_map[MAP_SIZE][MAP_SIZE];


// ------------------------------------------------------------
// Khởi tạo bản đồ client về trạng thái mặc định '-'
// ------------------------------------------------------------
void init_maps() {
    for (int r = 0; r < MAP_SIZE; ++r)
        for (int c = 0; c < MAP_SIZE; ++c) {
            own_map[r][c] = '-';
            enemy_map[r][c] = '-';
        }
}


// ------------------------------------------------------------
// Hàm hiển thị bản đồ ra terminal.
// In số cột trên cùng + số dòng ở bên trái
// ------------------------------------------------------------
void render_maps(){
    // OWN MAP
    printf("\n--- OWN MAP ---\n");
    printf("  ");
    for(int j=0;j<MAP_SIZE;j++) printf("%3d", j+1);
    printf("\n");

    for(int i=0;i<MAP_SIZE;i++){
        printf("%3d ", i+1);
        for(int j=0;j<MAP_SIZE;j++) printf(" %c ", own_map[i][j]);
        printf("\n");
    }

    // ENEMY MAP
    printf("\n--- ENEMY MAP ---\n");
    printf("  ");
    for(int j=0;j<MAP_SIZE;j++) printf("%3d", j+1);
    printf("\n");

    for(int i=0;i<MAP_SIZE;i++){
        printf("%3d ", i+1);
        for(int j=0;j<MAP_SIZE;j++) printf(" %c ", enemy_map[i][j]);
        printf("\n");
    }

    fflush(stdout);
}


// ------------------------------------------------------------
// Hàm trim chuỗi: bỏ ký tự trắng thừa đầu/cuối
// ------------------------------------------------------------
static void trim_inplace(char *s) {
    // trim trái
    char *p = s;
    while(*p == ' ' || *p == '\t' || *p == '\r') p++;
    if(p != s) memmove(s, p, strlen(p)+1);

    // trim phải
    int L = strlen(s);
    while(L>0 && (s[L-1]==' ' || s[L-1]=='\t' || s[L-1]=='\r' || s[L-1]=='\n')) {
        s[L-1]='\0';
        --L;
    }
}


// ------------------------------------------------------------
// Thread nhận dữ liệu từ server
// - Phân tách theo '#'
// - STATE: phân tích bản đồ
// - Các message khác: lưu lại để in sau
// ------------------------------------------------------------
void *recv_thread(void *arg){
    char buff[BUFF_SIZE];

    // Dùng để chứa các message thông báo không phải STATE
    const int MAX_MSGS = 256;
    char msgs[MAX_MSGS][256];

    while(1){
        int n = recv(sockfd, buff, BUFF_SIZE-1, 0);
        if(n <= 0){
            printf("\nDisconnected from server.\n");
            exit(0);
        }
        buff[n] = '\0';

        // Cắt theo '#'
        char *saveptr = NULL;
        char *token = strtok_r(buff, "#", &saveptr);

        int saw_state = 0;     // Có nhận STATE không
        int msg_count = 0;     // Số lượng tin nhắn ngoài STATE

        // Bản đồ tạm, chỉ gán vào bản đồ chính khi STATE hoàn chỉnh
        char tmp_own[MAP_SIZE][MAP_SIZE];
        char tmp_enemy[MAP_SIZE][MAP_SIZE];

        // Khởi tạo tạm thành '-'
        for(int r=0;r<MAP_SIZE;r++)
            for(int c=0;c<MAP_SIZE;c++){
                tmp_own[r][c] = '-';
                tmp_enemy[r][c] = '-';
            }

        // ------------------------------------------------------------
        // Xử lý từng token
        // ------------------------------------------------------------
        while(token){
            trim_inplace(token);
            if(strlen(token) == 0) {
                token = strtok_r(NULL, "#", &saveptr);
                continue;
            }

            // --------------------------------------------------------
            // Nếu là STATE: parse bản đồ server gửi sang
            // --------------------------------------------------------
            if(strncmp(token, "STATE:", 6) == 0){
                saw_state = 1;

                // Copy phần sau STATE: vào biến statecopy để tách theo dòng
                char statecopy[BUFF_SIZE];
                strncpy(statecopy, token + 6, sizeof(statecopy)-1);
                statecopy[sizeof(statecopy)-1] = '\0';

                char *line_save = NULL;
                char *line = strtok_r(statecopy, "\n", &line_save);

                int map_sel = -1;  // 0 = own_map, 1 = enemy_map
                int row = 0;

                while(line){
                    trim_inplace(line);

                    // Dòng trống → bỏ
                    if(strlen(line) == 0){
                        line = strtok_r(NULL, "\n", &line_save);
                        continue;
                    }

                    // Xác định map đang đọc
                    if(strcmp(line, "OWN MAP") == 0){
                        map_sel = 0;
                        row = 0;
                    }
                    else if(strcmp(line, "ENEMY MAP") == 0){
                        map_sel = 1;
                        row = 0;
                    }
                    // Đọc 1 dòng map
                    else if(map_sel != -1 && row < MAP_SIZE){
                        int col = 0;
                        for(int i=0; i < (int)strlen(line) && col < MAP_SIZE; ++i){
                            char ch = line[i];
                            if(ch != ' '){
                                if(map_sel == 0) tmp_own[row][col] = ch;
                                else tmp_enemy[row][col] = ch;
                                col++;
                            }
                        }
                        row++;
                    }

                    line = strtok_r(NULL, "\n", &line_save);
                }
            }
            // --------------------------------------------------------
            // Không phải STATE → là tin nhắn thông thường (READY_OK, MATCH_START, RESULT:HIT...)
            // --------------------------------------------------------
            else {
                if(msg_count < MAX_MSGS){
                    strncpy(msgs[msg_count], token, sizeof(msgs[msg_count])-1);
                    msgs[msg_count][sizeof(msgs[msg_count])-1] = '\0';
                    msg_count++;
                }
            }

            token = strtok_r(NULL, "#", &saveptr);
        }

        // ------------------------------------------------------------
        // Nếu có STATE → cập nhật bản đồ thật + render 1 lần duy nhất
        // ------------------------------------------------------------
        if(saw_state){
            for(int r=0;r<MAP_SIZE;r++)
                for(int c=0;c<MAP_SIZE;c++){
                    own_map[r][c] = tmp_own[r][c];
                    enemy_map[r][c] = tmp_enemy[r][c];
                }

            render_maps();
        }

        // ------------------------------------------------------------
        // In các message còn lại dưới bản đồ
        // ------------------------------------------------------------
        for(int i=0;i<msg_count;i++){
            printf("%s\n", msgs[i]);
        }

        fflush(stdout);
    }

    return NULL;
}


// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
int main(){
    init_maps();

    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){ perror("socket"); exit(1); }

    // Config địa chỉ server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // Kết nối
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("connect");
        exit(1);
    }
    printf("Connected to server.\n");

    // Tạo thread nhận dữ liệu server
    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);
    pthread_detach(tid);

    char input[BUFF_SIZE];
    char msg[BUFF_SIZE];

    render_maps();

    // ------------------------------------------------------------
    // Vòng nhập lệnh từ người chơi
    // ------------------------------------------------------------
    while(1){
        printf("\nPLACE:length,x,y,dir   READY   FIRE:x,y   QUIT\n> ");
        if(fgets(input, BUFF_SIZE, stdin) == NULL) break;

        // Xóa ký tự newline
        input[strcspn(input, "\n")] = 0;

        // Đảm bảo an toàn chiều dài
        input[BUFF_SIZE-2] = '\0';

        msg[0] = '\0';
        strncat(msg, input, BUFF_SIZE-2);
        strncat(msg, "#", 1); // Theo protocol server → tất cả command phải có '#'

        send(sockfd, msg, strlen(msg), 0);

        // Đợi server phản hồi (để thread kịp nhận STATE và render)
        if(strncmp(input, "PLACE:", 6) == 0 || strcmp(input, "READY") == 0) {
            usleep(100000); // 100ms
        }

        if(strncmp(input, "QUIT", 4) == 0)
            break;
    }

    close(sockfd);
    return 0;
}

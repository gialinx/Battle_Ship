# Hướng dẫn tích hợp Matchmaking System

## Tổng quan

Hệ thống matchmaking đã được cải tiến với các tính năng:
- ✅ ELO-based matching linh hoạt theo thời gian chờ
- ✅ Bảo vệ người chơi mới (<10 trận)
- ✅ Ưu tiên đối thủ có ELO gần nhất
- ✅ Auto-cleanup timeout (>5 phút)
- ✅ Chỉ match với người đang trong queue

## Cơ chế hoạt động

### 1. Điểm ELO khởi đầu
- **User mới: 1000 điểm** (đã config sẵn trong database.c line 23)
- Tránh âm điểm ngay cả khi thua nhiều trận đầu

### 2. ELO Range theo thời gian
| Thời gian chờ | ELO Range | Ý nghĩa |
|---------------|-----------|---------|
| 0-10s | ±100 | Tìm đối thủ cùng level |
| 10-30s | ±200 | Mở rộng ra |
| 30-60s | ±300 | Mở rộng hơn |
| 60-120s | ±500 | Chấp nhận mọi đối thủ gần |
| >120s | Không giới hạn | Match bất kỳ ai |

### 3. Bảo vệ người mới
- Người chơi <10 trận CHỈ match với người <30 trận
- Tránh newbie gặp cao thủ (bad experience)
- Tránh cao thủ farm newbie (unfair)

## Tích hợp vào Server

### Bước 1: Include header
```c
#include "matchmaking.h"
```

### Bước 2: Khởi tạo khi server start
```c
mm_init();
```

### Bước 3: Xử lý lệnh FIND_MATCH từ client
```c
// Khi nhận FIND_MATCH# từ client
if (strncmp(buffer, "FIND_MATCH#", 11) == 0) {
    // Lấy thông tin user từ database
    UserProfile profile;
    db_get_user_profile(client->user_id, &profile);
    
    // Thêm vào queue
    int result = mm_add_player(
        client->user_id,
        profile.username,
        profile.elo_rating,
        profile.total_games
    );
    
    if (result == 0) {
        send(client->socket, "MM_JOINED#\n", 11, 0);
    } else {
        send(client->socket, "MM_ERROR#Already in queue\n", 27, 0);
    }
}
```

### Bước 4: Polling tìm match (trong main loop)
```c
// Mỗi 2-3 giây check matchmaking
static time_t last_mm_check = 0;
time_t now = time(NULL);

if (now - last_mm_check >= 2) {
    last_mm_check = now;
    
    // Cleanup timeout players
    mm_cleanup_timeout();
    
    // Tìm match cho từng người trong queue
    for (int i = 0; i < mm_get_queue_size(); i++) {
        int user_id = mm_queue.entries[i].user_id;
        int matched_user_id;
        
        if (mm_find_match(user_id, &matched_user_id)) {
            // Tìm được match!
            // Gửi thông báo cho cả 2 người
            
            // Tìm socket của 2 người
            ClientInfo* client1 = find_client_by_user_id(user_id);
            ClientInfo* client2 = find_client_by_user_id(matched_user_id);
            
            if (client1 && client2) {
                // Gửi thông báo match found
                char msg1[256], msg2[256];
                snprintf(msg1, sizeof(msg1), "MATCH_FOUND#%s\n", 
                         mm_queue.entries[i].username);
                snprintf(msg2, sizeof(msg2), "MATCH_FOUND#%s\n",
                         client2->username);
                
                send(client1->socket, msg2, strlen(msg2), 0);
                send(client2->socket, msg1, strlen(msg1), 0);
                
                // Remove cả 2 khỏi queue
                mm_remove_player(user_id);
                mm_remove_player(matched_user_id);
                
                // Bắt đầu game...
            }
        }
    }
}
```

### Bước 5: Xử lý CANCEL_MATCH
```c
if (strncmp(buffer, "CANCEL_MATCH#", 13) == 0) {
    mm_remove_player(client->user_id);
    send(client->socket, "MM_CANCELLED#\n", 14, 0);
}
```

### Bước 6: Remove khi disconnect
```c
// Khi client disconnect
void handle_client_disconnect(ClientInfo* client) {
    // Remove khỏi queue nếu đang chờ match
    if (mm_is_in_queue(client->user_id)) {
        mm_remove_player(client->user_id);
        printf("[MM] Player %s removed (disconnected)\n", client->username);
    }
    
    // ... cleanup khác
}
```

## API Functions

### mm_add_player()
```c
int mm_add_player(int user_id, const char* username, int elo_rating, int total_games);
```
- Thêm player vào queue
- Return 0 nếu thành công, -1 nếu lỗi (đã có trong queue hoặc queue đầy)

### mm_remove_player()
```c
int mm_remove_player(int user_id);
```
- Xóa player khỏi queue
- Return 0 nếu thành công, -1 nếu không tìm thấy

### mm_find_match()
```c
int mm_find_match(int user_id, int* matched_user_id);
```
- Tìm đối thủ phù hợp cho user_id
- Return 1 nếu tìm được (matched_user_id được set), 0 nếu chưa tìm được

### mm_is_in_queue()
```c
int mm_is_in_queue(int user_id);
```
- Kiểm tra user có trong queue không
- Return 1 nếu có, 0 nếu không

### mm_cleanup_timeout()
```c
void mm_cleanup_timeout();
```
- Tự động xóa người chờ >5 phút
- Nên gọi định kỳ trong main loop

### mm_get_queue_size()
```c
int mm_get_queue_size();
```
- Lấy số người đang chờ trong queue

### mm_print_queue()
```c
void mm_print_queue();
```
- Debug: In ra danh sách người đang chờ

## Protocol giữa Client-Server

### Client → Server
```
FIND_MATCH#          - Tham gia hàng chờ
CANCEL_MATCH#        - Hủy tìm trận
```

### Server → Client
```
MM_JOINED#           - Đã vào queue thành công
MM_ERROR#<reason>    - Lỗi (đã trong queue, queue đầy)
MM_CANCELLED#        - Đã hủy thành công
MATCH_FOUND#<opponent_name>  - Tìm được đối thủ!
MM_TIMEOUT#          - Timeout >5 phút, tự động remove
```

## Best Practices

1. **Luôn cleanup khi disconnect**: Prevent ghost entries
2. **Polling interval 2-3s**: Không quá thường xuyên, tránh lag
3. **Timeout 5 phút**: Tự động remove người AFK
4. **Log rõ ràng**: Dùng mm_print_queue() để debug
5. **Thread-safe**: Nếu dùng multi-thread, cần mutex cho mm_queue

## Testing

Dùng test_matchmaking.c để verify:
```bash
cd /home/chau/Battle_Ship/new_code/server
gcc test_matchmaking.c src/matchmaking.c -o test_mm -lm
./test_mm
```

## Troubleshooting

**Q: Người chơi không tìm được match?**
- Check ELO range: Dùng mm_print_queue() xem wait time và range
- Check newbie protection: Người <10 trận chỉ match với <30 trận
- Check queue size: Có đủ 2+ người không?

**Q: Match với người offline?**
- **Không thể xảy ra**: Chỉ người gọi FIND_MATCH mới vào queue
- Đảm bảo remove khi disconnect

**Q: Queue bị đầy?**
- Tăng MAX_QUEUE trong matchmaking.h (hiện tại: 20)
- Hoặc giảm MM_TIMEOUT_SECONDS để cleanup nhanh hơn

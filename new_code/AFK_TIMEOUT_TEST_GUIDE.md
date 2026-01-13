# 🧪 HƯỚNG DẪN TEST CHỨC NĂNG AFK TIMEOUT

## Chuẩn bị

```bash
# Terminal 1: Chạy server
cd /home/chau/Battle_Ship/new_code/server
./run_lan_server.sh

# Terminal 2: Chạy client 1
cd /home/chau/Battle_Ship/new_code/client
./run_lan_client.sh 127.0.0.1

# Terminal 3: Chạy client 2
cd /home/chau/Battle_Ship/new_code/client
./run_lan_client.sh 127.0.0.1
```

## Test Case 1: AFK Warning trong Placing Ships

**Mục đích**: Kiểm tra cảnh báo AFK khi đang đặt tàu

### Các bước:

1. **Client 1**: Đăng nhập (user1)
2. **Client 2**: Đăng nhập (user2)
3. **Client 1**: Gửi lời mời cho user2
4. **Client 2**: Chấp nhận lời mời
5. **Cả hai** vào màn hình đặt tàu
6. **Client 1**: Đặt 1-2 tàu rồi **DỪNG LẠI**
7. **Chờ 3 phút** (180 giây)

### ✅ Kết quả mong đợi:

- Sau **3 phút**: Popup AFK warning xuất hiện trên Client 1
- Popup hiển thị: "Are you still there? You have been inactive for 3 minutes."
- Nút "I'M HERE!" có thể click được

### Tiếp theo:

**A. Phản hồi kịp thời**:
- Click "I'M HERE!" trong vòng 2 phút
- ✅ Popup đóng, tiếp tục đặt tàu bình thường

**B. Không phản hồi**:
- **Không click** gì trong 2 phút tiếp theo (tổng 5 phút)
- ✅ Client 1 tự động forfeit
- ✅ Client 2 nhận thông báo "OPPONENT_LEFT_PLACEMENT"
- ✅ Cả hai về lobby, **không mất ELO** (vì chưa bắt đầu game)

## Test Case 2: AFK Warning trong Game

**Mục đích**: Kiểm tra AFK timeout khi đang chơi

### Các bước:

1. **Cả hai client** đặt xong tàu và click READY
2. Game bắt đầu
3. **Client 1** (nếu là lượt của mình): Bắn 1-2 phát
4. **Client 2** (nếu là lượt của mình): Bắn 1-2 phát
5. **Khi đến lượt Client 1**: **DỪNG LẠI, không bắn**
6. **Chờ 3 phút**

### ✅ Kết quả mong đợi:

- Sau **3 phút**: Popup AFK warning xuất hiện trên Client 1
- Server log: `[AFK] user1 has been idle for 180 seconds - sending warning`

### Tiếp theo:

**A. Phản hồi kịp thời**:
- Click "I'M HERE!" trong vòng 2 phút
- ✅ Popup đóng
- ✅ Tiếp tục chơi bình thường
- Client log: `CLIENT: Responded to AFK warning`
- Server log: `[AFK] user1 responded to AFK warning`

**B. Không phản hồi**:
- **Không click** trong 2 phút tiếp theo (tổng 5 phút)
- Server log: `[AFK] user1 has been AFK for 300 seconds - auto forfeiting`
- ✅ Client 1 **THUA** (forfeit)
- ✅ Client 2 **THẮNG**
- ✅ **ELO được cập nhật** (Client 1 -ELO, Client 2 +ELO)
- ✅ Match được lưu vào database với marker "FORFEIT"

## Test Case 3: Reset Timer khi hoạt động

**Mục đích**: Đảm bảo timer reset khi người chơi hoạt động

### Các bước:

1. Vào game và chơi
2. **Chờ 2 phút 30 giây** (gần đến 3 phút)
3. **Bắn 1 phát** (hoặc bất kỳ hành động nào)
4. **Chờ thêm 2 phút 30 giây**

### ✅ Kết quả mong đợi:

- **KHÔNG** có popup AFK warning xuất hiện
- Timer đã reset về 0 khi bắn
- Phải idle thêm 3 phút nữa mới có warning

## Test Case 4: Multiple AFK Warnings

**Mục đích**: Test nhiều lần warning

### Các bước:

1. Vào game
2. Chờ 3 phút → popup xuất hiện
3. Click "I'M HERE!"
4. **Chờ lại 3 phút**

### ✅ Kết quả mong đợi:

- Popup xuất hiện lần 2
- Có thể phản hồi nhiều lần
- Mỗi lần phản hồi reset timer

## Kiểm tra Database

Sau khi test case 2B (AFK forfeit trong game):

```bash
cd /home/chau/Battle_Ship/new_code/server
sqlite3 battleship.db

# Xem match history
SELECT match_id, player1_id, player2_id, winner_id, match_data 
FROM match_history 
ORDER BY played_at DESC 
LIMIT 5;

# Kiểm tra ELO changes
SELECT username, elo_rating FROM users;
```

Trong `match_data` phải có text "FORFEIT"

## Thời gian Test nhanh (DEV MODE)

Nếu muốn test nhanh, tạm thời giảm thời gian timeout:

### Chỉnh trong server_lobby.c (dòng 1995-1996):

```c
// Original (3 phút + 2 phút)
const int AFK_WARNING_TIMEOUT = 180;  // 3 minutes
const int AFK_FORFEIT_TIMEOUT = 300;  // 5 minutes

// Test mode (30 giây + 30 giây)
const int AFK_WARNING_TIMEOUT = 30;   // 30 seconds for testing
const int AFK_FORFEIT_TIMEOUT = 60;   // 1 minute total for testing
```

Sau đó biên dịch lại:
```bash
cd /home/chau/Battle_Ship/new_code/server
make clean && make
```

**⚠️ LƯU Ý**: Nhớ đổi lại về thời gian gốc sau khi test xong!

## Checklist

- [ ] Test AFK warning trong placing ships
- [ ] Test phản hồi "I'M HERE!" trong placing ships
- [ ] Test auto forfeit trong placing ships (không mất ELO)
- [ ] Test AFK warning trong game
- [ ] Test phản hồi "I'M HERE!" trong game
- [ ] Test auto forfeit trong game (có mất ELO)
- [ ] Test reset timer khi hoạt động
- [ ] Kiểm tra server logs
- [ ] Kiểm tra database có lưu đúng không
- [ ] Test nhiều lần warning liên tiếp

## Debug Tips

### Server logs quan trọng:

```
[AFK] username has been idle for 180 seconds - sending warning
[AFK] username responded to AFK warning
[AFK] username has been AFK for 300 seconds - auto forfeiting
[FORFEIT] username surrendered to opponent (Match ID=X, ELO change: -25 vs +25)
```

### Client logs quan trọng:

```
CLIENT: AFK warning received
CLIENT: Responded to AFK warning
```

### Nếu không thấy popup:

1. Kiểm tra server có gửi AFK_WARNING# không (xem server log)
2. Kiểm tra client có parse message không (xem client log)
3. Kiểm tra `game->afk_warning_visible` flag
4. Kiểm tra render có được gọi không

### Nếu timer không reset:

1. Kiểm tra `last_activity_time` có được update không
2. Mỗi action phải gửi message về server
3. Server phải update `last_activity_time` trong recv handler

## Kết luận

Chức năng AFK timeout hoạt động đầy đủ với:
- ✅ 3 phút idle → Warning
- ✅ 5 phút idle → Auto forfeit
- ✅ Popup đẹp, rõ ràng
- ✅ Tích hợp ELO system
- ✅ Lưu database
- ✅ Thread chạy nền hiệu quả

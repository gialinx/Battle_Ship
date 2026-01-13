# Hướng dẫn Test Chức năng Xử lý Disconnect

## Tính năng mới

Server giờ đây có khả năng xử lý khi người chơi bị disconnect đột ngột trong lúc chơi (do mất mạng, hết pin, crash, v.v.)

### Cách hoạt động

Khi một người chơi bị disconnect:

1. **Kiểm tra trạng thái game**: Server kiểm tra xem người chơi có đang trong trận đấu không
2. **Phân biệt giai đoạn**:
   - **Đang đặt tàu**: Hủy trận, không tính điểm ELO
   - **Đang chơi**: Tính như surrender/forfeit
3. **Cập nhật kết quả**:
   - Người còn lại: **THẮNG** (nhận điểm ELO tăng)
   - Người disconnect: **THUA** (mất điểm ELO)
4. **Lưu vào database**: Ghi nhận match với marker "DISCONNECT"

## Cách test

### Test 1: Disconnect trong lúc đặt tàu

1. Khởi động server:
   ```bash
   cd /home/chau/Battle_Ship/new_code/server
   ./run_server.sh
   ```

2. Mở 2 client và đăng nhập bằng 2 tài khoản khác nhau

3. Gửi lời mời và chấp nhận để vào màn hình đặt tàu

4. **Tắt mạng** hoặc **đóng cửa sổ** của 1 client (giả lập disconnect)

5. **Kết quả mong đợi**:
   - Client còn lại nhận message: `OPPONENT_LEFT_PLACEMENT:username`
   - Trở về lobby
   - KHÔNG có thay đổi ELO
   - Server log: `[DISCONNECT] username disconnected during placement vs opponent (no ELO change)`

### Test 2: Disconnect trong lúc chơi

1. Khởi động server và 2 client

2. Tạo trận đấu, đặt tàu và bắt đầu chơi (cả 2 đều READY)

3. Chơi một vài lượt

4. **Tắt mạng** hoặc **kill process** của 1 client (giả lập disconnect)

5. **Kết quả mong đợi**:
   - Client còn lại nhận: `GAME_OVER:WIN:Opponent disconnected:NEW_ELO:+CHANGE`
   - ELO được cập nhật (người còn lại +ELO, người disconnect -ELO)
   - Match được lưu vào database với marker "DISCONNECT"
   - Server log: `[DISCONNECT] username disconnected from match vs opponent (Match ID=X, ELO change: +Y)`

### Test 3: Disconnect bằng cách kill process

```bash
# Terminal 1: Chạy client
cd /home/chau/Battle_Ship/new_code/client
./run_client.sh

# Terminal 2: Tìm process ID
ps aux | grep client_gui

# Sau khi vào game, kill process
kill -9 <PID>
```

## Kiểm tra trong Database

```bash
cd /home/chau/Battle_Ship/new_code/server
./check_db.py
```

Tìm trong match history xem có match với `match_data` chứa "DISCONNECT" không.

## So sánh với Surrender

| Tình huống | Người chơi | Kết quả | Lưu database |
|-----------|-----------|---------|--------------|
| **Surrender** | Người surrender | Thua, -ELO | "FORFEIT" |
| **Disconnect** | Người disconnect | Thua, -ELO | "DISCONNECT" |
| Cả hai | Đối thủ | Thắng, +ELO | Match saved |

## Lưu ý

- Disconnect trong placement: **KHÔNG ảnh hưởng ELO**
- Disconnect trong game: **CÓ ảnh hưởng ELO** (như surrender)
- Server tự động cleanup và reset game state
- Đối thủ được thông báo ngay lập tức
- Match history được lưu đầy đủ cho cả hai trường hợp

## Debug

Nếu có vấn đề, kiểm tra server logs để xem:
- `[DISCONNECT]` messages
- Match ID được tạo
- ELO changes
- Database save status

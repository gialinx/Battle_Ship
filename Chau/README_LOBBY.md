# BATTLESHIP GAME - HƯỚNG DẪN SỬ DỤNG

## 📋 Tổng quan hệ thống

### Các thành phần:
1. **server_lobby** - Server chính với hệ thống lobby và mời chơi
2. **client_gui_login** - Client GUI với đăng nhập/đăng ký và lobby
3. **database (SQLite)** - Lưu trữ user và lịch sử trận đấu với ELO rating

---

## 🚀 CÁCH CHẠY

### Bước 1: Compile
```bash
make clean
make
```

### Bước 2: Khởi động Server
```bash
./server_lobby
```

### Bước 3: Chạy Client (mở terminal mới)
```bash
./client_gui_login
```

Bạn có thể mở nhiều client cùng lúc để test multiplayer.

---

## 🎮 FLOW CHƠI GAME

### 1. Màn hình ĐĂNG NHẬP/ĐĂNG KÝ
- Nhập **username** và **password**
- Click **ĐĂNG KÝ** để tạo tài khoản mới
- Click **ĐĂNG NHẬP** để vào game
- Hoặc click **Tạo tài khoản** / **Đã có tài khoản?** để chuyển đổi

**User test có sẵn:**
- Username: `player1` | Password: `pass1` (ELO: 1200)
- Username: `player2` | Password: `pass2` (ELO: 1100)

---

### 2. Màn hình LOBBY
Sau khi đăng nhập thành công, bạn sẽ thấy:

- **Header**: Thông tin cá nhân (username, ELO, số trận, số thắng)
- **Danh sách người chơi**: Tất cả user online/offline
  - Tên người chơi + ELO rating
  - Status: `online` (màu xanh) hoặc `offline` (màu xám)
  - Nút **"Mời chơi"** (chỉ với người online)

**Các chức năng:**
- **Click vào người chơi** để chọn
- **Click "Mời chơi"** để gửi lời mời
- **Click "Làm mới"** để cập nhật danh sách
- **Click "Đăng xuất"** để logout

---

### 3. Hệ thống MỜI CHƠI

#### Người mời (Player A):
1. Click nút **"Mời chơi"** bên cạnh tên đối thủ
2. Popup hiện ra: "Đang chờ phản hồi..."
3. Có thể **Hủy** để quay lại lobby

#### Người nhận (Player B):
1. Popup hiện ra: "Lời mời chơi game!"
2. Message: "**username** muốn mời bạn chơi game"
3. 2 options:
   - **Đồng ý** → Bắt đầu game
   - **Từ chối** → Quay lại lobby

---

### 4. Màn hình CHƠI GAME
(Phần này đang trong quá trình phát triển)

Khi cả 2 người **Đồng ý**:
- Chuyển sang màn hình đặt tàu
- Sau khi đặt xong → Bắt đầu chơi
- Kết thúc game → Tự động lưu kết quả và tính ELO

---

## 📊 HỆ THỐNG ELO RATING

### Công thức tính:
```
ΔRA = round(K × (SA - EA))
```

Trong đó:
- **K**: Hệ số K (40 cho người mới, 20 cho trung bình, 10 cho cao thủ)
- **SA**: Actual Score (0-1, tính theo kết quả + hiệu suất)
- **EA**: Expected Score (xác suất thắng dựa trên chênh lệch ELO)

### Ví dụ:
- Player A (ELO: 1200) **thắng** Player B (ELO: 1000) → +15 ELO
- Player A (ELO: 1200) **thua** Player B (ELO: 1000) → -25 ELO

---

## 🗄️ KIỂM TRA DATABASE

### Xem danh sách users:
```bash
sqlite3 battleship.db "SELECT * FROM users;"
```

### Xem lịch sử trận đấu:
```bash
sqlite3 battleship.db "SELECT * FROM match_history;"
```

### Tạo user mới:
```bash
sqlite3 battleship.db "INSERT INTO users (username, password) VALUES ('newuser', 'password123');"
```

---

## 🐛 DEBUG

### Kiểm tra server đang chạy:
```bash
ps aux | grep server_lobby
```

### Kill server:
```bash
pkill server_lobby
```

### Xóa database và reset:
```bash
rm battleship.db
./server_lobby  # Tự động tạo lại database
```

---

## 📝 PROTOCOL MESSAGES

### Client → Server:
- `REGISTER:username:password#` - Đăng ký
- `LOGIN:username:password#` - Đăng nhập
- `GET_USERS#` - Lấy danh sách người chơi
- `INVITE:user_id#` - Mời người chơi
- `ACCEPT_INVITE:user_id#` - Chấp nhận lời mời
- `DECLINE_INVITE:user_id#` - Từ chối lời mời
- `CANCEL_INVITE#` - Hủy lời mời
- `LOGOUT#` - Đăng xuất

### Server → Client:
- `LOGIN_OK:username:games:wins:elo:user_id#` - Đăng nhập thành công
- `USERS:count:id1,name1,status1,elo1:...#` - Danh sách users
- `INVITE_FROM:user_id:username#` - Nhận lời mời
- `INVITE_ACCEPTED#` - Lời mời được chấp nhận
- `INVITE_DECLINED#` - Lời mời bị từ chối
- `GAME_START#` - Bắt đầu trận đấu

---

## 🎯 TODO (Tính năng sắp có)

- [ ] Màn hình đặt tàu trong GUI
- [ ] Màn hình chơi game hoàn chỉnh
- [ ] Chat trong game
- [ ] Xem lịch sử trận đấu
- [ ] Leaderboard (bảng xếp hạng)
- [ ] Replay trận đấu
- [ ] Sound effects
- [ ] Animation

---

## 📧 Liên hệ
Nếu có bug hoặc câu hỏi, vui lòng tạo issue trên GitHub!

**Chúc bạn chơi game vui vẻ! 🎮🚢**

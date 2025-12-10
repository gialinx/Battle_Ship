# BATTLESHIP GAME - GUI VERSION (SDL2)

## 📁 Cấu trúc project

```
Battle_Ship_game/
├── src/
│   ├── client_gui.c      ✅ Client mới có giao diện SDL2
│   ├── network.c         ✅ Logic socket tách riêng
│   └── network.h         ✅ Header file
├── assets/
│   ├── images/           (Chưa dùng)
│   └── fonts/            (Chưa dùng)
├── client.c              ✅ Client terminal cũ (backup)
├── client                Binary client terminal
├── client_gui            ✅ Binary client GUI mới
├── server.c              ✅ Server (giữ nguyên)
├── server                Binary server
├── Makefile              ✅ Đã cập nhật
└── run_gui.sh            ✅ Script chạy GUI
```

## 🎮 Cách sử dụng

### 1. Compile tất cả
```bash
make clean
make
```

### 2. Chạy server
```bash
./server
```

### 3. Chạy client GUI
Terminal 1:
```bash
./client_gui
```

Terminal 2:
```bash
./client_gui
```

## ⚠️ LƯU Ý QUAN TRỌNG

**Hiện tại client GUI chưa có màn hình đặt tàu!**

Để test, bạn cần:
1. Chạy `./client` cũ (terminal) trên 2 terminal
2. Đặt tàu và gõ READY trên cả 2
3. Sau đó đóng client cũ, chạy `./client_gui` để chơi với giao diện đẹp

## ✅ Đã hoàn thành

- [x] Cài đặt SDL2, SDL2_ttf, SDL2_image
- [x] Tách logic network thành module riêng
- [x] Màn hình chơi game (PLAYING STATE):
  - Vẽ 2 bản đồ 13x13 (Own Map + Enemy Map)
  - Hiển thị số hàng/cột
  - Màu sắc:
    - 🌊 Xanh dương = Nước
    - ⬜ Xám = Tàu của bạn
    - ⚪ Trắng + X = Bắn trượt
    - 🔴 Đỏ + O = Bắn trúng
    - ⚫ Đen + O = Tàu chìm
  - Click chuột vào bản đồ đối thủ để bắn
  - Hiển thị lượt chơi (YOUR TURN / Chờ đối thủ)
  - Hiển thị message thông báo
- [x] Xử lý nhận STATE từ server
- [x] Xử lý FIRE command
- [x] Makefile hỗ trợ compile SDL2

## 🚧 Chưa hoàn thành

- [ ] Màn hình đăng nhập (Login)
- [ ] Màn hình đặt tàu (Placing Ships) - QUAN TRỌNG!
- [ ] Màn hình chờ đối thủ (Waiting)
- [ ] Màn hình kết thúc (Game Over)
- [ ] Animation (nổ, sóng...)
- [ ] Âm thanh
- [ ] Font chữ tiếng Việt đẹp hơn

## 🎨 Màn hình hiện tại

### Gameplay Screen
```
┌─────────────────────────────────────────────────┐
│  BATTLESHIP - TRAN DAU                         │
│  LUOT CUA BAN! / Cho doi thu...                │
├─────────────────────────────────────────────────┤
│  BAN DO CUA BAN          BAN DO DOI THU        │
│  [Lưới 13x13]            [Lưới 13x13]          │
│   - Tàu: màu xám          - Ẩn tàu đối thủ     │
│   - x: trượt              - x: trượt           │
│   - o: trúng              - o: trúng           │
│   - @: chìm               - @: chìm            │
│                                                 │
│  Message: [Thông báo...]                       │
└─────────────────────────────────────────────────┘
```

## 🔧 Debug

Nếu gặp lỗi font:
```bash
# Kiểm tra font có tồn tại không
ls /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf

# Nếu không có, cài đặt:
sudo apt-get install fonts-dejavu-core
```

## 📝 TODO Tiếp theo

1. **QUAN TRỌNG**: Thêm màn hình đặt tàu
2. Thêm màn hình login
3. Thêm màn hình game over với thống kê
4. Cải thiện UI/UX
5. Thêm animation và âm thanh

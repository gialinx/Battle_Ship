# Battle Ship - Client

Client GUI cho game Battleship multiplayer (sử dụng SDL2).

## Yêu cầu hệ thống

- GCC compiler
- SDL2, SDL2_ttf, SDL2_image, SDL2_mixer libraries
- X11 hoặc Wayland display server (cho Linux)

## Cài đặt dependencies

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev build-essential
```

### Font (nếu thiếu):
```bash
sudo apt install fonts-dejavu
```

## Build

```bash
make
```

Sẽ tạo file binary: `client_gui`

## Chạy client

### Cách 1: Dùng script (khuyến nghị)
```bash
./run_client.sh
```

Script sẽ hỏi địa chỉ IP và port của server.

### Cách 2: Chạy thủ công
```bash
./client_gui
```

**Lưu ý**: Cần sửa địa chỉ server trong code nếu server không chạy trên localhost.

## Kết nối tới server từ xa

Nếu server chạy trên máy khác, cần cấu hình địa chỉ IP trong file:
- `src/network/network.c` - Thay đổi `SERVER_IP` và `SERVER_PORT`

Hoặc sử dụng script `run_client.sh` để nhập địa chỉ khi chạy.

## Tài khoản test

- Username: `player1` / Password: `pass1`
- Username: `player2` / Password: `pass2`

(Cần tạo trong database server trước)

## Lỗi thường gặp

### 1. Không kết nối được server
```
Error: Connection refused
```
→ Kiểm tra server có đang chạy không, và địa chỉ IP/port đúng chưa.

### 2. Lỗi font
```
Error: Couldn't load font
```
→ Cài đặt fonts: `sudo apt install fonts-dejavu`

### 3. Lỗi SDL (WSL)
```
Error: Couldn't initialize SDL
```
→ Kiểm tra DISPLAY environment variable hoặc dùng WSLg (Windows 11)

## Assets (Ảnh và Âm thanh)

Client hỗ trợ tích hợp ảnh và âm thanh vào game!

### Thêm assets:
1. Đặt file ảnh (.png, .jpg) vào: `assets/images/`
2. Đặt file âm thanh (.wav, .mp3) vào: `assets/sounds/`
3. Load trong code (xem hướng dẫn chi tiết)

### Tài liệu:
- **[assets/README.md](assets/README.md)** - Hướng dẫn cơ bản
- **[assets/EXAMPLE_USAGE.md](assets/EXAMPLE_USAGE.md)** - Ví dụ chi tiết từng bước

### Ví dụ nhanh:
```c
// Load ảnh
assets_load_image(&game.assets, game.renderer, "explosion.png");

// Vẽ ảnh
assets_render_image(renderer, &game->assets, "explosion.png", x, y, 50, 50);

// Phát âm thanh
assets_play_sound(&game->assets, "shot.wav", 0);
```

## Deployment trên máy khác

1. Copy toàn bộ folder `client/` sang máy client
2. Cài đặt dependencies (bao gồm SDL2_mixer)
3. Build: `make`
4. Cấu hình địa chỉ server trong code hoặc dùng script
5. Chạy: `./run_client.sh`

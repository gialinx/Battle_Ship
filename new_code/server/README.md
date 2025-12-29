# Battle Ship - Server

Server component cho game Battleship multiplayer.

## Yêu cầu hệ thống

- GCC compiler
- SQLite3 development library
- pthread support

## Cài đặt dependencies

```bash
sudo apt-get update
sudo apt-get install -y libsqlite3-dev build-essential
```

## Build

```bash
make
```

Sẽ tạo ra 2 file binary:
- `server` - Game server chính
- `server_lobby` - Lobby server (quản lý đăng nhập, matchmaking)

## Chạy server

### Cách 1: Dùng script (khuyến nghị)
```bash
./run_server.sh
```

### Cách 2: Chạy thủ công
```bash
./server_lobby
```

Server sẽ lắng nghe trên **port 8888**.

## Cấu hình

- Port mặc định: 8888
- Database: `battleship.db` (tự động tạo khi chạy lần đầu)
- Max clients: 10

## Database

Server sử dụng SQLite để lưu:
- Thông tin người dùng (username, password)
- ELO rating
- Lịch sử trận đấu
- Thống kê (wins/losses/total games)

**Vị trí database**: `battleship.db` sẽ được tạo trong thư mục `server/` khi chạy lần đầu.

**Lưu ý**:
- Database được tạo tự động khi server khởi động lần đầu
- Database **KHÔNG** bị xóa khi chạy lại server (dữ liệu được giữ nguyên)
- Khi backup server, nhớ backup cả file `battleship.db` để giữ dữ liệu users

### Reset database (xóa tất cả users và lịch sử):
```bash
# Cách 1: Dùng Makefile
make clean-db

# Cách 2: Xóa thủ công
rm battleship.db

# Sau đó chạy lại server để tạo database mới
./run_server.sh
```

## Dừng server

```bash
pkill server_lobby
```

hoặc `Ctrl+C` nếu chạy trong terminal.

## Deployment trên máy chủ khác

1. Copy toàn bộ folder `server/` sang máy chủ
2. Cài đặt dependencies
3. Build: `make`
4. Chạy: `./run_server.sh`

**Lưu ý**: Mở port 8888 trên firewall nếu cần client từ máy khác kết nối.

# Battle Ship Game - Phiên bản Modular

Game Battleship multiplayer với kiến trúc client-server, được tách riêng để dễ deploy trên các máy khác nhau.

## Cấu trúc project

```
new_code/
├── server/           → Server component (có thể chạy trên máy riêng)
│   ├── src/
│   │   ├── database.c/h       - Quản lý SQLite database
│   │   └── server_lobby.c     - Lobby server
│   ├── server.c               - Game server chính
│   ├── Makefile
│   ├── run_server.sh          - Script chạy server
│   └── README.md
│
└── client/           → Client GUI (có thể chạy trên máy khác)
    ├── src/
    │   ├── client/            - Main client logic
    │   ├── ui/                - SDL2 UI (screens, renderer)
    │   ├── network/           - Network protocol, socket handling
    │   └── core/              - Core data structures
    ├── Makefile
    ├── run_client.sh          - Script chạy client
    └── README.md
```

## Quick Start

### 1. Chạy trên cùng một máy (Local testing)

**Terminal 1 - Server:**
```bash
cd server/
./run_server.sh
```

**Terminal 2 - Client:**
```bash
cd client/
./run_client.sh
# Nhập: 127.0.0.1 (localhost)
```

### 2. Chạy trên 2 máy khác nhau (Production)

**Trên máy Server:**
```bash
# Copy folder server/ sang máy server
scp -r server/ user@server-ip:/path/to/

# SSH vào máy server
ssh user@server-ip
cd /path/to/server/
./run_server.sh

# Mở port 8888 trên firewall (nếu cần)
sudo ufw allow 8888/tcp
```

**Trên máy Client:**
```bash
# Copy folder client/ sang máy client
scp -r client/ user@client-ip:/path/to/

# SSH vào máy client (hoặc chạy trực tiếp)
cd /path/to/client/
./run_client.sh
# Nhập IP của máy server khi được hỏi
```

## Dependencies

### Server:
- gcc, make
- libsqlite3-dev
- pthread

### Client:
- gcc, make
- libsdl2-dev
- libsdl2-ttf-dev
- libsdl2-image-dev
- fonts-dejavu

## Kiến trúc

- **Server**: Sử dụng TCP socket, pthread để xử lý multiple clients
- **Client**: SDL2 cho GUI, TCP socket để kết nối server
- **Database**: SQLite (chỉ server cần, lưu users, ELO, match history)
- **Protocol**: Text-based protocol với delimiter '#'

## Tính năng

- ✅ Đăng nhập/Đăng ký
- ✅ Lobby với danh sách người chơi online
- ✅ Hệ thống mời chơi (invite/accept/decline)
- ✅ Đặt tàu chiến
- ✅ Chơi game (turn-based)
- ✅ ELO rating system
- ✅ Lịch sử trận đấu

## Ports

- Server lobby: **8888** (mặc định)
- Game server: **9999** (tùy chỉnh trong code)

## Tài khoản test

- `player1` / `pass1`
- `player2` / `pass2`

## Troubleshooting

Xem chi tiết trong:
- [server/README.md](server/README.md)
- [client/README.md](client/README.md)

## Development

Code đã được comment đầy đủ bằng tiếng Việt (đặc biệt là phần UI).

### Build từng phần:
```bash
# Build server
cd server/ && make

# Build client
cd client/ && make
```

### Clean:
```bash
# Clean server
cd server/ && make clean

# Clean client
cd client/ && make clean
```

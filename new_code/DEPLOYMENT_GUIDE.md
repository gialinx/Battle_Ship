# Hướng Dẫn Deploy Battleship Lên Nhiều Máy

## Tổng Quan

Game này hỗ trợ chạy trên nhiều laptop khác nhau qua mạng LAN hoặc Internet:
- **1 Laptop chạy Server**: Quản lý database, matchmaking, gameplay
- **2+ Laptop chạy Client**: Giao diện người chơi

---

## Yêu Cầu Hệ Thống

### Server (Laptop 1):
- **OS**: Ubuntu/Debian hoặc WSL2 trên Windows
- **Cần**: `gcc`, `make`, `libsqlite3-dev`, `libpthread`
- **Port**: Mở port 5500 (TCP)

### Client (Laptop 2, 3):
- **OS**: Ubuntu/Debian hoặc WSL2 trên Windows
- **Cần**: `gcc`, `make`, `libsdl2-dev`, `libsdl2-ttf-dev`, `libsdl2-image-dev`, `libsdl2-mixer-dev`
- **Fonts**: DejaVu Sans (thường có sẵn)

---

## Chuẩn Bị Code

### Option 1: Copy Binary Files (Nhanh nhất)

Trên máy dev (máy hiện tại):
```bash
cd /home/chau/Battle_Ship/Battle_Ship/new_code

# Build server
cd server
make clean && make

# Build client
cd ../client
make clean && make
```

**Copy sang các laptop**:
- **Server binary**: Copy `server/server_lobby` sang Laptop Server
- **Client binary**: Copy `client/client_gui` + folder `client/assets/` sang Laptop Client

### Option 2: Git Clone (Nếu các laptop có môi trường dev)

Trên mỗi laptop:
```bash
# Clone repo
git clone <your-repository-url>
cd Battle_Ship/Battle_Ship/new_code

# Trên laptop server:
cd server && make

# Trên laptop client:
cd client && make
```

---

## Cấu Hình Mạng

### Tình Huống 1: Cùng Mạng LAN (WiFi/Router)

**Bước 1: Tìm IP của Laptop Server**

Trên laptop server (WSL hoặc Linux):
```bash
# Nếu dùng WSL:
ip addr show eth0 | grep "inet "

# Hoặc:
hostname -I
```

Ví dụ output: `192.168.1.100`

**Bước 2: Mở Firewall trên Laptop Server**

**Trên Linux native**:
```bash
sudo ufw allow 5500/tcp
# Hoặc
sudo iptables -A INPUT -p tcp --dport 5500 -j ACCEPT
```

**Trên Windows (nếu chạy WSL)**:
1. Mở **Windows Defender Firewall**
2. **Advanced Settings** → **Inbound Rules** → **New Rule**
3. **Port** → **TCP** → Port `5500` → **Allow**

**Bước 3: Configure WSL Networking (Windows 11 only)**

Nếu dùng WSL2 trên Windows 11, tạo file `C:\Users\<YourName>\.wslconfig`:
```ini
[wsl2]
networkingMode=mirrored
firewall=false
```

Sau đó restart WSL:
```powershell
# Trong PowerShell (Windows):
wsl --shutdown
wsl
```

**Lưu ý**: Trên Windows 10, WSL2 dùng NAT - cần port forwarding:
```powershell
# Trong PowerShell (Administrator):
netsh interface portproxy add v4tov4 listenport=5500 listenaddress=0.0.0.0 connectport=5500 connectaddress=<WSL_IP>
```

Tìm WSL IP:
```bash
# Trong WSL:
ip addr show eth0 | grep "inet "
```

### Tình Huống 2: Khác Mạng (Internet)

**Cần thêm**:
1. **Port Forwarding** trên router của laptop server:
   - Login vào router (thường `192.168.1.1`)
   - Port Forwarding: External 5500 → Internal `<Server_IP>`:5500

2. **IP Public**: Tìm IP public của mạng server:
   ```bash
   curl ifconfig.me
   ```

3. **Dynamic DNS** (nếu IP public thay đổi):
   - Dùng NoIP, DuckDNS, v.v.

**Hoặc dùng VPN**:
- **Hamachi**, **ZeroTier**: Tạo virtual LAN

---

## Chạy Game

### Trên Laptop Server:

```bash
cd /home/chau/Battle_Ship/Battle_Ship/new_code/server
./server_lobby
```

Output sẽ hiện:
```
Database initialized successfully
Server listening on 0.0.0.0:5500 (all interfaces)
Waiting for connections...
```

### Trên Laptop Client 1:

**Cú pháp**:
```bash
./client_gui [server_ip]
```

**Ví dụ**:
```bash
# Kết nối tới localhost (test):
./client_gui

# Kết nối tới server LAN:
./client_gui 192.168.1.100

# Kết nối tới server Internet:
./client_gui 45.123.45.67
```

### Trên Laptop Client 2:

Tương tự Client 1:
```bash
./client_gui 192.168.1.100
```

---

## Test Kết Nối

### Bước 1: Test Từ Client

Trước khi chạy game, test kết nối TCP:

```bash
# Test ping (WSL hoặc Linux):
ping <server_ip>

# Test port 5500:
nc -zv <server_ip> 5500
# hoặc
telnet <server_ip> 5500
```

Nếu thành công:
```
Connection to 192.168.1.100 5500 port [tcp/*] succeeded!
```

### Bước 2: Kiểm Tra Server Log

Khi client kết nối, server sẽ in:
```
New client connected
```

Khi client login, server sẽ in:
```
Client 1 (user_id: 5) logged in as: player1
```

---

## Troubleshooting

### Lỗi: "Connection failed to <IP>:5500"

**Nguyên nhân**:
1. Server chưa chạy
2. Firewall block port 5500
3. IP sai
4. Không cùng mạng

**Giải pháp**:
```bash
# Kiểm tra server đang chạy:
ps aux | grep server_lobby

# Kiểm tra port đang listen:
netstat -tuln | grep 5500
# hoặc (trên WSL):
ss -tuln | grep 5500

# Test từ chính server:
telnet localhost 5500
```

### Lỗi: "SDL_Init Error" hoặc "TTF_Init Error"

**Nguyên nhân**: Thiếu library

**Giải pháp**:
```bash
# Ubuntu/Debian:
sudo apt-get update
sudo apt-get install libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-image-2.0-0 libsdl2-mixer-2.0-0

# Kiểm tra fonts:
ls /usr/share/fonts/truetype/dejavu/
```

### Lỗi: WSL mirrored mode không hoạt động

**Giải pháp**: Dùng port forwarding (Windows 10) hoặc nâng cấp lên Windows 11.

### Lag khi mở nhiều client

**Giải pháp**:
1. Không chạy nhiều client trên cùng 1 laptop
2. Nếu dùng WSL, chạy client trên Linux native hoặc Windows native sẽ mượt hơn
3. Giảm FPS hoặc dùng software renderer (xem phần Optimization)

---

## Package Cho Deployment (Nâng Cao)

Tạo folder release để phân phối:

```bash
cd /home/chau/Battle_Ship/Battle_Ship/new_code

# Tạo structure
mkdir -p release/server release/client

# Copy server
cp server/server_lobby release/server/
cp server/battleship.db release/server/  # Database (nếu muốn preserve users)

# Copy client
cp client/client_gui release/client/
cp -r client/assets release/client/

# Tạo README
cat > release/README.txt << 'EOF'
=== Battleship Game ===

SERVER:
1. cd server
2. ./server_lobby

CLIENT:
1. cd client
2. ./client_gui <server_ip>
   Ví dụ: ./client_gui 192.168.1.100

EOF

# Zip
cd release
tar -czf battleship-game.tar.gz server/ client/ README.txt
```

Giờ có thể gửi `battleship-game.tar.gz` cho bạn bè:
```bash
# Extract trên laptop khác:
tar -xzf battleship-game.tar.gz
cd client
./client_gui 192.168.1.100
```

---

## Network Architecture

```
┌─────────────────┐       LAN/Internet       ┌─────────────────┐
│  Client 1       │                           │  Server         │
│  (Laptop 2)     │◄─────TCP 5500────────────►│  (Laptop 1)     │
│  192.168.1.101  │                           │  192.168.1.100  │
└─────────────────┘                           │                 │
                                              │  - Database     │
┌─────────────────┐                           │  - Matchmaking  │
│  Client 2       │◄─────TCP 5500────────────►│  - Game Logic   │
│  (Laptop 3)     │                           └─────────────────┘
│  192.168.1.102  │
└─────────────────┘
```

---

## Performance Tips

### Server:
- Chạy trên laptop có dây (Ethernet) thay vì WiFi
- Không chạy ứng dụng nặng khác cùng lúc
- Database file `battleship.db` nên trên SSD

### Client:
- Nếu dùng WSL, chạy X Server (VcXsrv, X410) có thể mượt hơn
- Đóng các ứng dụng SDL khác
- Dùng Linux native sẽ nhanh hơn WSL

---

## Security Notes

**Cảnh báo**: Server hiện tại KHÔNG có encryption!

Không nên:
- Deploy lên Internet public mà không có VPN/tunnel
- Dùng password quan trọng
- Lưu thông tin nhạy cảm

Nếu deploy production, cần thêm:
- TLS/SSL cho socket
- Password hashing (bcrypt)
- Rate limiting
- Input validation

---

## Summary

**Deployment nhanh nhất**:

1. **Laptop Server (WSL/Linux)**:
   ```bash
   cd server && ./server_lobby
   ```

2. **Laptop Client 1, 2**:
   ```bash
   ./client_gui <server_ip>
   ```

3. **Firewall**: Mở port 5500 trên laptop server

4. **WSL Windows 11**: Enable mirrored networking trong `.wslconfig`

That's it! Enjoy playing Battleship! 🚢
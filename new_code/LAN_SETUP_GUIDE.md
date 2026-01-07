# HƯỚNG DẪN CHẠY BATTLESHIP TRÊN MẠNG LAN

## Yêu cầu hệ thống

Tất cả các máy (server + clients) cần:
- Ubuntu Linux (hoặc Debian-based)
- Ping thông với nhau qua mạng
- Dependencies đã cài

## CÀI ĐẶT DEPENDENCIES

Chạy trên tất cả các máy:

```bash
sudo apt update
sudo apt install -y build-essential gcc libsdl2-dev libsdl2-ttf-dev \
    libsdl2-image-dev libsdl2-mixer-dev libsqlite3-dev git
```

## BƯỚC 1: SETUP SERVER

### 1.1 Xác định IP của máy server

```bash
ip addr show | grep "inet " | grep -v 127.0.0.1
```

Ví dụ output:
```
inet 192.168.1.100/24 brd 192.168.1.255 scope global eth0
```

→ IP server: **192.168.1.100**

### 1.2 Mở firewall (nếu có)

```bash
sudo ufw allow 5501/tcp
```

Hoặc tắt firewall tạm thời để test:
```bash
sudo ufw disable
```

### 1.3 Chạy server

**Cách 1: Dùng script tự động**
```bash
cd ~/Battle_Ship/new_code/server
./run_lan_server.sh
```

**Cách 2: Chạy thủ công**
```bash
cd ~/Battle_Ship/new_code/server
make server_lobby
./server_lobby
```

Server sẽ hiện:
```
=================================
Server listening on 0.0.0.0 (all interfaces):5501
=================================
```

## BƯỚC 2: SETUP CLIENTS

### 2.1 Test kết nối đến server

Từ máy client, test ping và port:

```bash
# Test ping
ping 192.168.1.100

# Test port (thay IP bằng IP server của bạn)
telnet 192.168.1.100 5501
```

Hoặc:
```bash
nc -zv 192.168.1.100 5501
```

### 2.2 Chạy client

**Cách 1: Dùng script tự động**
```bash
cd ~/Battle_Ship/new_code/client
./run_lan_client.sh 192.168.1.100
```

**Cách 2: Chạy thủ công**
```bash
cd ~/Battle_Ship/new_code/client
make
export SERVER_IP=192.168.1.100
./client_gui
```

## BƯỚC 3: TEST KẾT NỐI

1. Mở client trên máy 1
2. Login với username/password bất kỳ (sẽ tự tạo account)
3. Mở client trên máy 2
4. Login với username khác
5. Thử invite hoặc matchmaking để test

## TROUBLESHOOTING

### Lỗi "Connection refused"

**Nguyên nhân có thể:**
1. Server chưa chạy
2. Firewall chặn port 5501
3. IP sai
4. Network không kết nối

**Cách fix:**
```bash
# Trên server: kiểm tra server có chạy không
ps aux | grep server_lobby

# Kiểm tra port có mở không
sudo netstat -tulpn | grep 5501

# Tắt firewall tạm thời
sudo ufw disable

# Kiểm tra IP
ip addr show
```

### Lỗi "Cannot ping server"

```bash
# Kiểm tra network interface
ip link show

# Kiểm tra routing
ip route show

# Ping từ client đến server
ping <server_ip>
```

### Server không nhận connection

Kiểm tra server có listen trên đúng interface không:
```bash
sudo netstat -tulpn | grep :5501
```

Output cần thấy:
```
tcp  0  0 0.0.0.0:5501  0.0.0.0:*  LISTEN
```

Nếu thấy `127.0.0.1:5501` thì server chỉ listen localhost → cần rebuild server.

## CẤU HÌNH NÂNG CAO

### Chạy server trên IP cụ thể

```bash
export SERVER_IP=192.168.1.100
./server_lobby
```

### Chạy nhiều clients trên cùng máy

Mỗi client sẽ tự động connect đến server. Có thể mở nhiều terminal và chạy:

```bash
# Terminal 1
SERVER_IP=192.168.1.100 ./client_gui

# Terminal 2
SERVER_IP=192.168.1.100 ./client_gui
```

## KIỂM TRA KẾT NỐI

### Trên server

Xem danh sách clients đang kết nối (thêm vào server code nếu cần)

### Logs

Server sẽ in ra:
```
New client connected from 192.168.1.101:xxxxx
User logged in: player1
```

## LƯU Ý QUAN TRỌNG

1. **Database chung**: Tất cả clients sẽ dùng database của server → user accounts được lưu trên server
2. **Copy code**: Phải copy toàn bộ thư mục `Battle_Ship` sang các VM, không chỉ executable
3. **Assets**: Client cần có fonts và assets trong thư mục `client/assets/`
4. **Permissions**: Đảm bảo scripts có quyền execute: `chmod +x *.sh`

## VÍ DỤ SETUP 3 MÁY

**Máy 1 (Server):** IP 192.168.1.100
```bash
cd ~/Battle_Ship/new_code/server
./run_lan_server.sh
```

**Máy 2 (Client 1):** IP 192.168.1.101
```bash
cd ~/Battle_Ship/new_code/client  
./run_lan_client.sh 192.168.1.100
# Login: player1 / pass1
```

**Máy 3 (Client 2):** IP 192.168.1.102
```bash
cd ~/Battle_Ship/new_code/client
./run_lan_client.sh 192.168.1.100
# Login: player2 / pass2
```

Sau đó player1 có thể invite player2 hoặc dùng matchmaking!

## HỖ TRỢ

Nếu gặp vấn đề, kiểm tra:
1. Server logs
2. Client terminal output  
3. Network connectivity: `ping`, `telnet`, `nc`
4. Firewall: `sudo ufw status`
5. Port listening: `sudo netstat -tulpn | grep 5501`

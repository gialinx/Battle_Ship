# Server Commands - Hướng dẫn nhanh

## Build & Clean

```bash
# Build server (không xóa database)
make

# Clean binaries (GIỮ NGUYÊN database)
make clean

# Xóa RIÊNG database (cẩn thận!)
make clean-db

# Xóa TẤT CẢ (bao gồm database)
make clean-all
```

## Chạy server

```bash
# Cách khuyến nghị (tự động compile + chạy)
./run_server.sh

# Chạy thủ công
./server_lobby
```

## Quản lý database

### Xem users trong database
```bash
sqlite3 battleship.db "SELECT * FROM users;"
```

### Xem chi tiết users
```bash
sqlite3 battleship.db "SELECT username, elo_rating, total_games, wins FROM users;"
```

### Tạo user mới (thủ công)
```bash
sqlite3 battleship.db "INSERT INTO users (username, password) VALUES ('player1', 'pass1');"
```

### Reset database (XÓA TẤT CẢ DỮ LIỆU)
```bash
make clean-db
./run_server.sh  # Tạo lại database mới
```

### Backup database
```bash
# Backup
cp battleship.db battleship.db.backup

# Restore
cp battleship.db.backup battleship.db
```

## Dừng server

```bash
# Dừng server đang chạy
pkill server_lobby

# Hoặc Ctrl+C nếu chạy trong terminal
```

## Debug

### Xem server có đang chạy không
```bash
ps aux | grep server_lobby
```

### Xem port 8888 có được sử dụng không
```bash
netstat -tulpn | grep 8888
# hoặc
lsof -i :8888
```

### Xem log khi server chạy
```bash
./server_lobby 2>&1 | tee server.log
```

---

**Lưu ý quan trọng**:
- ✅ `make clean` - An toàn, chỉ xóa binary
- ⚠️ `make clean-db` - XÓA DATABASE! Mất tất cả users!
- ⚠️ `make clean-all` - XÓA TẤT CẢ!

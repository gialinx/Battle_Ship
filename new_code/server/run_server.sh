#!/bin/bash
# Script để chạy Battle Ship Server

echo "========================================="
echo "  BATTLESHIP SERVER"
echo "========================================="
echo ""

cd "$(dirname "$0")"

# Kiểm tra dependencies
echo "🔍 Kiểm tra dependencies..."
MISSING_DEPS=0

if ! dpkg -l | grep -q libsqlite3-dev; then
    echo "  ❌ libsqlite3-dev chưa cài"
    MISSING_DEPS=1
fi

if [ $MISSING_DEPS -eq 1 ]; then
    echo ""
    echo "⚠️  Thiếu dependencies! Chạy lệnh sau để cài đặt:"
    echo ""
    echo "sudo apt-get update"
    echo "sudo apt-get install -y libsqlite3-dev"
    echo ""
    read -p "Bạn có muốn cài đặt ngay bây giờ? (y/n): " choice
    if [ "$choice" = "y" ] || [ "$choice" = "Y" ]; then
        sudo apt-get update
        sudo apt-get install -y libsqlite3-dev
    else
        echo "Thoát..."
        exit 1
    fi
fi

echo "  ✅ Dependencies đã sẵn sàng"
echo ""

# Compile (chỉ rebuild nếu cần, không xóa database)
echo "🔨 Đang compile server..."
make

if [ $? -ne 0 ]; then
    echo "❌ Lỗi khi compile!"
    exit 1
fi

echo "✅ Compile thành công!"
echo ""

# Kiểm tra server đang chạy
echo "🔍 Kiểm tra server..."
if pgrep -f "server_lobby" > /dev/null; then
    echo "⚠️  Server đang chạy. Dừng server cũ..."
    pkill -9 -f "server_lobby"
    sleep 2
    if pgrep -f "server_lobby" > /dev/null; then
        echo "❌ Không thể dừng server cũ. Vui lòng chạy: pkill -9 server_lobby"
        exit 1
    fi
    echo "✅ Server cũ đã dừng"
fi

# Hiển thị database info trước khi chạy
if [ -f "battleship.db" ]; then
    echo ""
    echo "📊 Database users:"
    sqlite3 battleship.db "SELECT username, elo_rating, total_games, wins FROM users;" 2>/dev/null || echo "   (Chưa có users)"
    echo ""
fi

echo "========================================="
echo "  SERVER STARTING"
echo "========================================="
echo ""
echo "🚀 Khởi động server_lobby..."
echo "📋 Cấu hình:"
echo "   - IP: 127.0.0.1 (localhost)"
echo "   - Port: 5501"
echo "   - Database: battleship.db"
echo ""
echo "🛑 Để dừng server: Ctrl+C"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "🛑 Đang dừng server..."
    exit 0
}

trap cleanup INT TERM

# Chạy ở FOREGROUND (không có dấu &)
./server_lobby

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
    pkill -f "server_lobby"
    sleep 1
fi

# Khởi động server
echo "🚀 Khởi động server_lobby trên port 8888..."
./server_lobby &
SERVER_PID=$!
sleep 2

# Kiểm tra server đã khởi động thành công
if ps -p $SERVER_PID > /dev/null; then
    echo "✅ Server đã khởi động (PID: $SERVER_PID)"
    echo ""
    echo "========================================="
    echo "  SERVER ĐANG CHẠY"
    echo "========================================="
    echo ""
    echo "📋 Thông tin:"
    echo "   - Port: 8888"
    echo "   - Database: battleship.db"
    echo ""

    # Hiển thị database info
    if [ -f "battleship.db" ]; then
        echo "📊 Database users:"
        sqlite3 battleship.db "SELECT username, elo_rating, total_games, wins FROM users;" 2>/dev/null || echo "   (Chưa có users)"
    fi

    echo ""
    echo "🛑 Để dừng server: pkill server_lobby hoặc Ctrl+C"
    echo ""

    # Cleanup function
    cleanup() {
        echo ""
        echo "🛑 Đang dừng server..."
        kill $SERVER_PID 2>/dev/null
        exit 0
    }

    trap cleanup INT TERM

    # Giữ script chạy
    wait $SERVER_PID
else
    echo "❌ Lỗi: Không thể khởi động server!"
    exit 1
fi

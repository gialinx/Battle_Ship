#!/bin/bash
# Script để chạy Battle Ship Client

echo "========================================="
echo "  BATTLESHIP CLIENT"
echo "========================================="
echo ""

cd "$(dirname "$0")"

# Kiểm tra dependencies
echo "🔍 Kiểm tra dependencies..."
MISSING_DEPS=0

if ! dpkg -l | grep -q libsdl2-dev; then
    echo "  ❌ libsdl2-dev chưa cài"
    MISSING_DEPS=1
fi

if ! dpkg -l | grep -q libsdl2-ttf-dev; then
    echo "  ❌ libsdl2-ttf-dev chưa cài"
    MISSING_DEPS=1
fi

if ! dpkg -l | grep -q libsdl2-image-dev; then
    echo "  ❌ libsdl2-image-dev chưa cài"
    MISSING_DEPS=1
fi

if ! dpkg -l | grep -q libsdl2-mixer-dev; then
    echo "  ❌ libsdl2-mixer-dev chưa cài"
    MISSING_DEPS=1
fi

if [ $MISSING_DEPS -eq 1 ]; then
    echo ""
    echo "⚠️  Thiếu dependencies! Chạy lệnh sau để cài đặt:"
    echo ""
    echo "sudo apt-get update"
    echo "sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev"
    echo ""
    read -p "Bạn có muốn cài đặt ngay bây giờ? (y/n): " choice
    if [ "$choice" = "y" ] || [ "$choice" = "Y" ]; then
        sudo apt-get update
        sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev
    else
        echo "Thoát..."
        exit 1
    fi
fi

echo "  ✅ Dependencies đã sẵn sàng"
echo ""

# Compile
echo "🔨 Đang compile client..."
make clean
make

if [ $? -ne 0 ]; then
    echo "❌ Lỗi khi compile!"
    exit 1
fi

echo "✅ Compile thành công!"
echo ""

# Nhập địa chỉ server
echo "📋 Cấu hình kết nối:"
read -p "Nhập địa chỉ IP của server (mặc định: 127.0.0.1): " SERVER_IP
SERVER_IP=${SERVER_IP:-127.0.0.1}

read -p "Nhập port của server (mặc định: 8888): " SERVER_PORT
SERVER_PORT=${SERVER_PORT:-8888}

echo ""
echo "🔌 Kết nối tới: $SERVER_IP:$SERVER_PORT"
echo ""

# Export để client có thể đọc (nếu cần)
export BATTLESHIP_SERVER_IP=$SERVER_IP
export BATTLESHIP_SERVER_PORT=$SERVER_PORT

echo "========================================="
echo "  ĐĂNG NHẬP"
echo "========================================="
echo ""
echo "Sử dụng tài khoản test:"
echo "  - player1 / pass1"
echo "  - player2 / pass2"
echo ""
echo "🚀 Khởi động client..."
echo ""

# Chạy client
./client_gui

EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo ""
    echo "❌ Client thoát với lỗi (code: $EXIT_CODE)"
    echo ""
    echo "Lỗi thường gặp:"
    echo "  1. Không kết nối được server → Kiểm tra server có đang chạy không"
    echo "  2. Font lỗi → sudo apt install fonts-dejavu"
    echo "  3. SDL lỗi → Kiểm tra DISPLAY hoặc WSLg"
    echo ""
fi

exit $EXIT_CODE

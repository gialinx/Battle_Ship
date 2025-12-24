#!/bin/bash
# Script để chạy Battleship Lobby System

echo "========================================="
echo "  BATTLESHIP GAME - LOBBY SYSTEM"
echo "========================================="
echo ""

# Kiểm tra server đang chạy
if pgrep -f "server_lobby" > /dev/null; then
    echo "⚠️  Server đang chạy. Dừng server cũ..."
    pkill -f "server_lobby"
    sleep 1
fi

# Khởi động server
echo "🚀 Khởi động server..."
./server_lobby &
SERVER_PID=$!
sleep 2

# Kiểm tra server đã khởi động thành công
if ps -p $SERVER_PID > /dev/null; then
    echo "✅ Server đã khởi động (PID: $SERVER_PID)"
    echo ""
    echo "========================================="
    echo "  HƯỚNG DẪN:"
    echo "========================================="
    echo "1. Chạy client GUI:"
    echo "   ./client_gui_login"
    echo ""
    echo "2. Hoặc mở nhiều terminal và chạy:"
    echo "   Terminal 1: ./client_gui_login"
    echo "   Terminal 2: ./client_gui_login"
    echo ""
    echo "3. Đăng nhập với:"
    echo "   - User: player1 | Pass: pass1"
    echo "   - User: player2 | Pass: pass2"
    echo ""
    echo "4. Để dừng server:"
    echo "   pkill server_lobby"
    echo "========================================="
    echo ""
    echo "📊 Database info:"
    sqlite3 battleship.db "SELECT username, elo_rating, total_games, wins FROM users;" 2>/dev/null || echo "No users yet"
    echo ""
    echo "Server đang chạy... Press Ctrl+C to stop"
    echo ""
    
    # Theo dõi log
    tail -f /dev/null
else
    echo "❌ Lỗi: Không thể khởi động server!"
    exit 1
fi

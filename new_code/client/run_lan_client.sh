#!/bin/bash
# Script to run Battleship client and connect to LAN server

echo "========================================="
echo "  BATTLESHIP CLIENT - LAN MODE"
echo "========================================="
echo ""

# Check if server IP is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <server_ip>"
    echo ""
    echo "Example:"
    echo "  $0 192.168.1.100"
    echo ""
    echo "Or set SERVER_IP environment variable:"
    echo "  SERVER_IP=192.168.1.100 $0"
    echo ""
    exit 1
fi

SERVER_IP=$1

echo "🌐 Server IP: $SERVER_IP"
echo "🔌 Port: 5501"
echo ""

# Test connection
echo "🔍 Testing connection to $SERVER_IP:5501..."
timeout 3 bash -c "echo > /dev/tcp/$SERVER_IP/5501" 2>/dev/null

if [ $? -eq 0 ]; then
    echo "   ✅ Server is reachable"
else
    echo "   ❌ Cannot connect to server!"
    echo ""
    echo "Possible issues:"
    echo "  1. Server is not running"
    echo "  2. Wrong IP address"
    echo "  3. Firewall blocking connection"
    echo "  4. Network issues"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo ""
echo "🔨 Compiling client..."
make
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi
echo "   ✅ Compilation successful"
echo ""

# Run client with server IP
echo "🚀 Starting client..."
echo "========================================="
echo ""

# Pass server IP as command line argument (more reliable than env var)
./client_gui $SERVER_IP

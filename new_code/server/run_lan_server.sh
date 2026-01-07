#!/bin/bash
# Script to run Battleship server on LAN

echo "========================================="
echo "  BATTLESHIP SERVER - LAN MODE"
echo "========================================="
echo ""

# Get server's IP address
SERVER_IP=$(ip addr show | grep "inet " | grep -v 127.0.0.1 | head -1 | awk '{print $2}' | cut -d/ -f1)

if [ -z "$SERVER_IP" ]; then
    echo "⚠️  Warning: Could not detect network IP"
    echo "   Server will listen on all interfaces (0.0.0.0)"
    echo ""
else
    echo "🌐 Network IP detected: $SERVER_IP"
    echo "   Clients should connect to: $SERVER_IP"
    echo ""
fi

# Check if firewall is blocking
if command -v ufw &> /dev/null; then
    UFW_STATUS=$(sudo ufw status | grep "Status: active")
    if [ ! -z "$UFW_STATUS" ]; then
        echo "🔥 Firewall is active. Opening port 5501..."
        sudo ufw allow 5501/tcp
        echo "   ✅ Port 5501 opened"
        echo ""
    fi
fi

# Compile server
echo "🔨 Compiling server..."
make server_lobby
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi
echo "   ✅ Compilation successful"
echo ""

# Run server
echo "🚀 Starting server..."
echo "   Press Ctrl+C to stop"
echo "========================================="
echo ""

# Server will listen on all interfaces by default
./server_lobby

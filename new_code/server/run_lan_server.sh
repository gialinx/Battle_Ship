#!/bin/bash
# Script to run Battleship server on LAN

echo "========================================="
echo "  BATTLESHIP SERVER - LAN MODE"
echo "========================================="
echo ""

# Check if SERVER_IP is already set
if [ -z "$SERVER_IP" ]; then
    # Auto-detect IP (prefer 10.0.2.x if available)
    AUTO_IP=$(ip addr show | grep "inet " | grep "10.0.2\." | head -1 | awk '{print $2}' | cut -d/ -f1)
    if [ -z "$AUTO_IP" ]; then
        # Fallback to any non-localhost IP
        AUTO_IP=$(ip addr show | grep "inet " | grep -v 127.0.0.1 | head -1 | awk '{print $2}' | cut -d/ -f1)
    fi
    
    if [ -z "$AUTO_IP" ]; then
        echo "⚠️  Could not auto-detect network IP"
        read -p "Enter server IP manually (or press Enter for 0.0.0.0): " SERVER_IP
    else
        echo "🌐 Detected IP: $AUTO_IP"
        read -p "Use this IP or enter manually [Y/n/IP]: " CHOICE
        if [ "$CHOICE" = "n" ] || [ "$CHOICE" = "N" ]; then
            read -p "Enter server IP: " SERVER_IP
        elif [[ "$CHOICE" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            SERVER_IP="$CHOICE"
        else
            SERVER_IP="$AUTO_IP"
        fi
    fi
fi

if [ -z "$SERVER_IP" ]; then
    echo "   Server will listen on all interfaces (0.0.0.0)"
    echo ""
else
    echo "✅ Using server IP: $SERVER_IP"
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

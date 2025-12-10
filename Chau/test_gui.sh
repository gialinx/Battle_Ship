#!/bin/bash
# Test script with debug output

echo "=== BATTLESHIP GUI TEST ==="
echo "Starting server and client..."
echo ""

# Start server in background
./server &
SERVER_PID=$!
sleep 1

echo "Server started (PID: $SERVER_PID)"
echo "Now run: ./client_gui"
echo ""
echo "Watch this terminal for debug messages!"
echo "Ships should appear in GRAY on the map after placement."
echo ""
echo "Press Ctrl+C to stop server"

wait $SERVER_PID

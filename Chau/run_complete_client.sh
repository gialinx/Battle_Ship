#!/bin/bash
# Wrapper to run complete Battleship experience
# Combines login + game clients seamlessly

CHAU_DIR="/home/giali/Github_clone/Battle_Ship/Battle_Ship/Chau"
cd "$CHAU_DIR"

SESSION_FILE="/tmp/battleship_session_$$"
GAME_PID_FILE="/tmp/battleship_game_pid_$$"

cleanup() {
    rm -f "$SESSION_FILE" "$GAME_PID_FILE"
    pkill -P $$ 2>/dev/null
}

trap cleanup EXIT

echo "=================================="
echo "  BATTLESHIP - Complete Client"
echo "=================================="
echo ""

# Start with login/lobby
echo "Starting lobby client..."
./client_gui_login

# When login exits, check if game should start
if [ -f "$SESSION_FILE" ]; then
    echo "Game session found, launching game..."
    
    # Launch game client with session
    ./client_gui &
    GAME_PID=$!
    echo $GAME_PID > "$GAME_PID_FILE"
    
    # Wait for game to finish
    wait $GAME_PID
    
    # Return to lobby
    echo "Game finished, returning to lobby..."
    rm -f "$SESSION_FILE"
    exec "$0"  # Restart this script
fi

echo "Goodbye!"

#!/bin/bash
# Merge client_gui_login with client_gui_v2 to create complete client

echo "Merging login/lobby screen with game screen..."
echo "This will create a complete Battleship client with:"
echo "  1. Login/Register screen"
echo "  2. Lobby with user list"
echo "  3. Invitation system"
echo "  4. Ship placement (from client_gui_v2.c)"
echo "  5. Game play"
echo ""
echo "The new client will be: client_gui_complete"
echo ""

cd /home/giali/Github_clone/Battle_Ship/Battle_Ship/Chau

# Backup existing files
echo "Creating backups..."
cp src/client_gui_login.c src/client_gui_login.c.backup
cp src/client_gui_v2.c src/client_gui_v2.c.backup

echo ""
echo "To integrate, we need to:"
echo "1. Keep login/lobby states and UI from client_gui_login.c"
echo "2. Import ship placement and game logic from client_gui_v2.c"
echo "3. Connect STATE_PLACING_SHIPS to use client_gui_v2's placement system"
echo ""
echo "This requires manual code integration."
echo "Would you like me to create the merged file? (This will take a few moments)"

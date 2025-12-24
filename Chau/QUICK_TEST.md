# QUICK TEST GUIDE - Integrated Client

## 🚀 Quick Start

### 1. Compile
```bash
cd /home/giali/Github_clone/Battle_Ship/Battle_Ship/Chau
make clean
make server_lobby client_gui_login
```

### 2. Start Server
**Terminal 1:**
```bash
./server_lobby
```
Expect output:
```
Database initialized successfully
Server listening at 127.0.0.1:5500
```

### 3. Start Players
**Terminal 2 (Player 1):**
```bash
DISPLAY=:0 ./client_gui_login
```

**Terminal 3 (Player 2):**
```bash
DISPLAY=:0 ./client_gui_login
```

---

## 🎮 Test Flow

### Step 1: Login
**Both Players:**
- Player 1: username=`player1`, password=`pass1`
- Player 2: username=`player2`, password=`pass2`
- Click "Login" button

✅ **Expected**: Lobby screen với user list

### Step 2: Invite
**Player 2:**
- Click on "player1" in user list (should highlight)
- Click "Invite" button

✅ **Expected**: 
- Player 2 sees "Waiting for response..."
- Player 1 sees dialog "player2 wants to play"

### Step 3: Accept
**Player 1:**
- Click "Accept" button in dialog

✅ **Expected**: Both players see "DAT TAU - BATTLESHIP" screen

### Step 4: Place Ships
**Both Players (independently):**

1. **Select ship** - Click buttons on left:
   - "Lon (4o) [0/1]" → selects 4-cell ship
   - "Vua (3o) [0/1]" → selects 3-cell ship  
   - "Nho (2o) [0/2]" → selects 2-cell ship (need 2)

2. **Rotate** - Press `R` key to toggle horizontal/vertical

3. **Preview** - Move mouse over grid:
   - Green overlay = valid placement
   - Red overlay = invalid (overlap or out of bounds)

4. **Place** - Click on grid to place ship:
   - Ship appears as gray cells
   - Counter updates: "Da dat: 1/4 tau"
   - Selected ship resets

5. **Repeat** until "Da dat: 4/4 tau"

6. **Ready** - Click "READY!" button

✅ **Expected Messages**:
- First player ready: "Waiting for opponent to READY..."
- Second player ready: "Opponent is READY! Click READY when done."

### Step 5: Game Starts
**After both READY:**

✅ **Expected**: 
- Screen changes to "BATTLESHIP - TRAN DAU"
- Left map shows YOUR ships (gray cells)
- Right map shows ENEMY grid (no ships visible)
- Message: "Game started! Good luck!"

---

## 🐛 Troubleshooting

### UI Issues

**Ships overlap or layout broken:**
- ✅ FIXED: Used cell_display=28 instead of CELL_SIZE=30
- ✅ FIXED: Moved ship list to x=20, buttons width=150

**Font too bold:**
- ✅ FIXED: Changed from `game.font` to `game.font_small`

**Can't click buttons:**
- Check coordinates match render positions
- Current: Ship list at (20, 100+i*55)
- Map at (200, 100)
- READY at (map_x+150, map_y+364)

### READY Not Working

**Symptom**: Click READY but nothing happens

**Checks**:
```bash
# Server side
grep "is READY" server.log
grep "Both players ready" server.log

# Should see:
# player1 is READY
# player2 is READY  
# Both players ready! Game starting: player1 vs player2
```

**Fix**: Make sure server compiled with new handle_ready()

### Network Issues

**Client can't connect:**
```bash
# Check server running
ps aux | grep server_lobby

# Check port
netstat -tlnp | grep 5500
```

**Messages not received:**
```bash
# Client output should show:
RECEIVED: WELCOME#
RECEIVED: LOGIN_OK:player1:0:0:1200:1#
RECEIVED: INVITE_ACCEPTED#
RECEIVED: GAME_START#
RECEIVED: START_PLAYING#
```

---

## 📊 Visual Indicators

### Ship Placement Screen
```
✓ Title: "DAT TAU - BATTLESHIP" (normal font, not bold)
✓ Left panel: Ship buttons with [count/max]
✓ Green checkmark ✓ for completed ships
✓ Yellow border on selected ship
✓ Grid: 13x13 cells, 28px each
✓ Preview: Green=valid, Red=invalid
✓ READY: Gray (disabled) or Blue (enabled)
✓ Bottom: "Da dat: X/4 tau" counter
```

### Playing Screen
```
✓ Title: "BATTLESHIP - TRAN DAU"
✓ Turn indicator: "LUOT CUA BAN!" (green) or "Cho doi thu..." (yellow)
✓ Left map: YOUR ships visible (gray)
✓ Right map: ENEMY ships hidden
✓ Hits: Orange-red with yellow dot
✓ Misses: White with black X
```

---

## 🔑 Keyboard Shortcuts

- `R` - Rotate ship (only in PLACING_SHIPS state)
- `Q` - Quit to lobby (any time in game)
- `TAB` - Switch between username/password fields (login screen)
- `ENTER` - Submit login/register

---

## 📝 Test Checklist

### Login Flow
- [ ] Login with valid credentials → Lobby
- [ ] Login with wrong password → Error message
- [ ] Register new account → Success
- [ ] Toggle Login/Register button

### Lobby
- [ ] User list shows online/offline status
- [ ] ELO ratings displayed
- [ ] Click user highlights
- [ ] Invite button works
- [ ] Refresh updates user list
- [ ] Logout returns to login

### Ship Placement
- [ ] All 3 ship types selectable
- [ ] R key rotates ship
- [ ] Preview shows green when valid
- [ ] Preview shows red when invalid
- [ ] Can't place overlapping ships
- [ ] Counter updates after placing
- [ ] READY enabled after 4 ships
- [ ] READY sends message to server

### Game Start
- [ ] First READY → "Waiting for opponent..."
- [ ] Second READY → Both get START_PLAYING
- [ ] Screen switches to playing mode
- [ ] Own ships visible on left
- [ ] Enemy map hidden on right

### Current Status
⚠️ **Note**: Full gameplay (firing, hit detection) requires additional server logic not yet implemented in server_lobby.c. Current integration covers:
- ✅ Login/Register
- ✅ Lobby/Invite
- ✅ Ship Placement
- ✅ Transition to Playing
- ❌ Actual firing mechanics (needs game server merge)

---

## 🎯 Next Development Steps

1. **Merge server_lobby.c with server.c**
   - Combine invitation system + game logic
   - Handle FIRE, HIT, MISS, SUNK messages

2. **Implement turn management**
   - Server decides who goes first
   - Send YOUR_TURN / OPPONENT_TURN

3. **Game over handling**
   - Detect when all ships sunk
   - Send GAME_OVER with winner
   - Update ELO ratings in database

4. **Return to lobby**
   - Reset game state
   - Allow rematch or new opponent

---

**Last Updated**: 2024-12-24
**Integration Status**: ✅ UI Complete, ⚠️ Game Logic Pending

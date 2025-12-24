# Integration Guide: Login/Lobby + Ship Placement

## Current Situation

We have 2 separate client programs:

### 1. `client_gui_login` (src/client_gui_login.c)
✅ Login/Register screen  
✅ Lobby with user list  
✅ Invitation system  
❌ Temporary game screen (just placeholder)

### 2. `client_gui` (src/client_gui_v2.c)  
❌ No login system  
✅ Ship placement UI  
✅ Game playing UI  
✅ Turn-based shooting

## Integration Options

### Option 1: Simple Launcher (QUICKEST) ⭐
Modify `client_gui_login.c` to launch `client_gui` when game starts:

**Pros:**
- Quick to implement (5 minutes)
- Minimal code changes
- Both programs work independently

**Cons:**
- 2 separate windows
- Need to pass socket/user info between processes

**Implementation:**
```c
// In client_gui_login.c, when INVITE_ACCEPTED:
else if(strncmp(buffer, "INVITE_ACCEPTED", 15) == 0) {
    // Save game info to file
    FILE* f = fopen("/tmp/battleship_session.txt", "w");
    fprintf(f, "%d\n%s\n%d\n", game.sockfd, game.my_username, game.my_user_id);
    fclose(f);
    
    // Launch game client
    system("./client_gui &");
    
    // Close lobby client
    game.running = 0;
}
```

### Option 2: Full Integration (BEST) ⭐⭐⭐
Merge both files into one complete client:

**Pros:**
- Single unified application
- Smooth state transitions
- Professional UX

**Cons:**
- More coding work (1-2 hours)
- Large file (~1500 lines)

**Steps:**
1. Copy ship placement functions from `client_gui_v2.c` to `client_gui_login.c`
2. Replace temporary `render_game_screen()` with actual implementation
3. Add ship data structures to Game struct
4. Import rendering and input handling

### Option 3: Modular Architecture (CLEANEST) ⭐⭐
Separate into modules and link:

```
client_gui_complete:
  - main.c (state machine)
  - login_ui.c (login/register)
  - lobby_ui.c (user list, invites)
  - game_ui.c (ship placement, playing)
  - network.c (already exists)
  - database.c (already exists)
```

## Recommended: Option 1 (Quick Launcher)

Let me implement this for you:

### Step 1: Modify client_gui_v2.c to read session
Add to beginning of main():
```c
// Read session info if exists
FILE* f = fopen("/tmp/battleship_session.txt", "r");
if(f) {
    fscanf(f, "%d\n%49s\n%d\n", &sockfd, username, &user_id);
    fclose(f);
    remove("/tmp/battleship_session.txt");
    printf("Loaded session: %s (ID: %d)\n", username, user_id);
}
```

### Step 2: Modify client_gui_login.c to launch game
Replace the temporary game screen code with launcher.

### Step 3: Test
```bash
./client_gui_login  # Login and invite
# When accepted -> auto launches game window
# After game -> return to lobby
```

## Which option do you prefer?

1. **Quick Launcher** (5 min) - 2 windows, works immediately
2. **Full Integration** (1-2 hours) - Single unified app  
3. **Modular** (2-3 hours) - Clean architecture

Let me know and I'll implement it!

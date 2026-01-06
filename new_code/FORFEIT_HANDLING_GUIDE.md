# Hướng dẫn xử lý Forfeit và Quit Game

## 📋 Tổng quan vấn đề

Hiện tại chương trình thiếu logic xử lý khi:
1. **Player đang đặt tàu** ấn "Back to Lobby" 
2. **Player đang chơi game** muốn thoát/forfeit

## 🎯 Giải pháp đề xuất

### 1. Màn hình đặt tàu (PLACING_SHIPS) - Ấn "Back to Lobby"

**Vấn đề:**
- Hiện tại chỉ reset local state
- Đối thủ vẫn đang chờ
- Server không biết player đã rời đi

**Giải pháp:**
- Hiển thị **confirmation dialog** trước khi thoát
- Gửi lệnh `FORFEIT` cho server
- Server thông báo cho đối thủ: "Opponent left during ship placement"
- Cả 2 player về lobby, không tính ELO

**Màn hình xác nhận:**
```
┌─────────────────────────────────────┐
│     LEAVE SHIP PLACEMENT?           │
│                                     │
│  Are you sure you want to leave?   │
│  Your opponent will be notified.   │
│                                     │
│   [STAY]         [LEAVE]            │
└─────────────────────────────────────┘
```

---

### 2. Màn hình đang chơi (PLAYING) - Forfeit game

**Vấn đề:**
- Không có nút "Forfeit" hoặc "Exit"
- Player không thể thoát nếu muốn đầu hàng

**Giải pháp:**
- Thêm nút **"Forfeit Game"** ở góc màn hình
- Hiển thị **confirmation dialog** nghiêm túc
- Gửi lệnh `FORFEIT` cho server
- Server xử lý:
  - Player forfeit → **LOSE** (trừ ELO)
  - Đối thủ → **WIN** (cộng ELO)
  - Lưu match vào database
- Hiển thị kết quả forfeit

**Màn hình xác nhận:**
```
┌─────────────────────────────────────┐
│       ⚠️  FORFEIT GAME?             │
│                                     │
│  You will LOSE this match and       │
│  your ELO will decrease!            │
│                                     │
│  This cannot be undone.             │
│                                     │
│   [CANCEL]       [FORFEIT]          │
└─────────────────────────────────────┘
```

---

## 🔧 Implementation Plan

### A. Client-side Changes

#### 1. Thêm màn hình Confirmation Dialog

**File mới:** `confirmation_dialog.c/h`

```c
typedef enum {
    DIALOG_NONE,
    DIALOG_FORFEIT_PLACEMENT,  // Thoát khi đang đặt tàu
    DIALOG_FORFEIT_GAME        // Forfeit khi đang chơi
} DialogType;

typedef struct {
    DialogType type;
    int visible;
    char title[100];
    char message[200];
    char button1_text[20];  // "CANCEL" hoặc "STAY"
    char button2_text[20];  // "LEAVE" hoặc "FORFEIT"
} ConfirmationDialog;
```

#### 2. Update placing_ships_screen.c

**Thêm vào placing_ships_handle_click:**
```c
// Click BACK button
int back_x = MAP_X - 40, back_y = MAP_Y + MAP_SIZE * CELL_DISPLAY + 20;
if(x >= back_x && x <= back_x + 150 && y >= back_y && y <= back_y + 50) {
    // Show confirmation dialog
    game->confirmation_dialog.type = DIALOG_FORFEIT_PLACEMENT;
    game->confirmation_dialog.visible = 1;
    strcpy(game->confirmation_dialog.title, "LEAVE SHIP PLACEMENT?");
    strcpy(game->confirmation_dialog.message, 
           "Are you sure you want to leave?\nYour opponent will be notified.");
    strcpy(game->confirmation_dialog.button1_text, "STAY");
    strcpy(game->confirmation_dialog.button2_text, "LEAVE");
    return;  // Don't leave immediately
}
```

#### 3. Update playing_screen.c

**Thêm nút Forfeit:**
```c
void playing_screen_render(SDL_Renderer* renderer, GameData* game) {
    // ... existing code ...
    
    // FORFEIT button (bottom-left corner)
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    int forfeit_x = 30, forfeit_y = 640;
    int forfeit_hover = (mx >= forfeit_x && mx <= forfeit_x + 150 && 
                         my >= forfeit_y && my <= forfeit_y + 50);
    
    render_button(renderer, game->font_small, "Forfeit Game", 
                  forfeit_x, forfeit_y, 150, 50,
                  (SDL_Color){180, 0, 0, 255},  // Dark red
                  forfeit_hover, 1);
}
```

**Thêm handler:**
```c
void playing_screen_handle_click(GameData* game, int x, int y) {
    // ... existing code ...
    
    // Click FORFEIT button
    int forfeit_x = 30, forfeit_y = 640;
    if(x >= forfeit_x && x <= forfeit_x + 150 && 
       y >= forfeit_y && y <= forfeit_y + 50) {
        // Show confirmation dialog
        game->confirmation_dialog.type = DIALOG_FORFEIT_GAME;
        game->confirmation_dialog.visible = 1;
        strcpy(game->confirmation_dialog.title, "⚠️  FORFEIT GAME?");
        strcpy(game->confirmation_dialog.message,
               "You will LOSE this match and\nyour ELO will decrease!\n\nThis cannot be undone.");
        strcpy(game->confirmation_dialog.button1_text, "CANCEL");
        strcpy(game->confirmation_dialog.button2_text, "FORFEIT");
        return;
    }
}
```

#### 4. Update main event loop

**File:** `client_gui_complete.c`

```c
// Handle confirmation dialog clicks
if(game.confirmation_dialog.visible) {
    confirmation_dialog_handle_click(&game, x, y);
    break;
}

// Normal screen handling
switch(game.state) {
    // ... existing cases ...
}
```

---

### B. Server-side Changes

#### 1. Handle FORFEIT command

**File:** `server_lobby.c`

```c
void handle_forfeit(Client* client) {
    if(!client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    Client* opponent = find_client_by_user_id(client->opponent_id);
    
    if(!opponent) {
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    // Determine game phase
    int during_placement = (!client->is_ready || !opponent->is_ready);
    
    if(during_placement) {
        // During placement - no ELO change
        send_to_client(client->fd, "FORFEIT_PLACEMENT#");
        send_to_client(opponent->fd, "OPPONENT_LEFT_PLACEMENT#");
        
        // Reset both players
        reset_game_state(client);
        reset_game_state(opponent);
        
        printf("%s forfeited during placement vs %s\n", 
               client->username, opponent->username);
    } else {
        // During game - record as loss
        int winner_elo = opponent->elo_rating;
        int loser_elo = client->elo_rating;
        
        // Calculate ELO changes
        int winner_new_elo, loser_new_elo;
        calculate_elo(winner_elo, loser_elo, 1, &winner_new_elo, &loser_new_elo);
        
        int winner_change = winner_new_elo - winner_elo;
        int loser_change = loser_new_elo - loser_elo;
        
        // Update database
        db_update_user_stats(opponent->user_id, 1, winner_new_elo);  // Win
        db_update_user_stats(client->user_id, 0, loser_new_elo);     // Loss
        
        // Save match to database
        time_t now = time(NULL);
        int duration = (now - client->game_start_time);
        
        db_save_match(
            opponent->user_id,   // winner
            client->user_id,     // loser
            opponent->total_shots,
            opponent->total_hits,
            client->total_shots,
            client->total_hits,
            duration,
            opponent->shot_log,
            client->shot_log,
            winner_elo,
            winner_new_elo,
            loser_elo,
            loser_new_elo
        );
        
        // Send results
        char win_msg[512];
        snprintf(win_msg, sizeof(win_msg),
                "GAME_OVER:WIN:Opponent forfeited:%d:%d#",
                winner_new_elo, winner_change);
        send_to_client(opponent->fd, win_msg);
        
        char lose_msg[512];
        snprintf(lose_msg, sizeof(lose_msg),
                "GAME_OVER:LOSE:You forfeited:%d:%d#",
                loser_new_elo, loser_change);
        send_to_client(client->fd, lose_msg);
        
        printf("%s FORFEITED game vs %s (ELO: %d→%d)\n",
               client->username, opponent->username, loser_elo, loser_new_elo);
        
        // Reset both players
        reset_game_state(client);
        reset_game_state(opponent);
    }
    
    pthread_mutex_unlock(&clients_lock);
}

void reset_game_state(Client* client) {
    client->in_game = 0;
    client->opponent_id = -1;
    client->is_ready = 0;
    client->is_my_turn = 0;
    client->ship_count = 0;
    client->total_shots = 0;
    client->total_hits = 0;
    client->game_start_time = 0;
    memset(client->shot_log, 0, sizeof(client->shot_log));
    init_map(client->map);
    init_map(client->enemy_map);
}
```

#### 2. Update command parser

```c
else if(strncmp(buffer, "FORFEIT", 7) == 0) {
    handle_forfeit(client);
}
```

---

## 🎨 UI/UX Design

### Màn hình Confirmation Dialog

**Vị trí:** Center overlay (modal)
**Kích thước:** 400x250 pixels
**Style:**

```
Background: Semi-transparent black overlay (0,0,0,180)
Dialog box: Dark blue (40,50,70)
Border: Cyan (0,200,255)
Title: Large, yellow/red depending on severity
Message: White, center-aligned
Buttons: 
  - Cancel/Stay: Green (0,200,100)
  - Leave/Forfeit: Red (200,0,0)
```

### Visual Hierarchy

1. **Title** - Large, attention-grabbing
2. **Warning icon** - For forfeit game (⚠️)
3. **Message** - Clear consequences
4. **Buttons** - High contrast, clear labels

---

## 📊 Protocol Messages

### Client → Server

| Command | Format | Description |
|---------|--------|-------------|
| FORFEIT | `FORFEIT#` | Player forfeits current game |

### Server → Client

| Response | Format | Description |
|----------|--------|-------------|
| FORFEIT_PLACEMENT | `FORFEIT_PLACEMENT#` | You left during placement |
| OPPONENT_LEFT_PLACEMENT | `OPPONENT_LEFT_PLACEMENT#` | Opponent left during placement |
| GAME_OVER (forfeit) | `GAME_OVER:LOSE:You forfeited:elo:change#` | Forfeit loss result |
| GAME_OVER (opponent forfeit) | `GAME_OVER:WIN:Opponent forfeited:elo:change#` | Win by opponent forfeit |

---

## ✅ Testing Checklist

### Test Cases

- [ ] **TC1**: Player A đặt tàu, ấn Back → Dialog hiện
- [ ] **TC2**: Player A ấn "STAY" → Dialog đóng, tiếp tục đặt tàu
- [ ] **TC3**: Player A ấn "LEAVE" → Gửi FORFEIT, về lobby
- [ ] **TC4**: Player B nhận thông báo "Opponent left"
- [ ] **TC5**: Cả 2 về lobby, ELO không đổi
- [ ] **TC6**: Player A đang chơi, ấn "Forfeit Game" → Dialog hiện
- [ ] **TC7**: Player A ấn "CANCEL" → Dialog đóng, tiếp tục chơi
- [ ] **TC8**: Player A ấn "FORFEIT" → Gửi FORFEIT
- [ ] **TC9**: Player A nhận GAME_OVER:LOSE, ELO giảm
- [ ] **TC10**: Player B nhận GAME_OVER:WIN, ELO tăng
- [ ] **TC11**: Match được lưu vào database
- [ ] **TC12**: Cả 2 về lobby sau forfeit

---

## 🚀 Ưu điểm của giải pháp

1. ✅ **User-friendly**: Xác nhận trước khi thoát
2. ✅ **Fair**: ELO chỉ thay đổi khi forfeit trong game
3. ✅ **Clear feedback**: Thông báo rõ ràng cho cả 2 player
4. ✅ **Data integrity**: Lưu match vào database
5. ✅ **Consistent**: Xử lý giống như game kết thúc bình thường

---

## 📝 Notes

- **Placement phase**: Không tính ELO (game chưa bắt đầu thực sự)
- **Playing phase**: Tính ELO đầy đủ (coi như thua)
- **Confirmation dialog**: Prevent accidental quits
- **Server validation**: Always validate game state trước khi forfeit


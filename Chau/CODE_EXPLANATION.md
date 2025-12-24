# CODE INTEGRATION - GIẢI THÍCH CHI TIẾT

## Tổng quan Integration
Project này merge 2 GUI clients riêng biệt thành 1 ứng dụng hoàn chỉnh:
- **client_gui_login.c**: Login/Register/Lobby system
- **client_gui_v2.c**: Ship placement và gameplay UI

## 1. KIẾN TRÚC TỔNG THỂ

### Flow Diagram
```
┌─────────────┐
│ STATE_LOGIN │ ← Màn hình đăng nhập/đăng ký
└──────┬──────┘
       │ (login thành công)
       ▼
┌─────────────┐
│ STATE_LOBBY │ ← Danh sách user online, chọn đối thủ
└──────┬──────┘
       │ (accept invite)
       ▼
┌──────────────────┐
│STATE_PLACING_SHIPS│ ← Đặt 4 tàu trên lưới 13x13
└──────┬───────────┘
       │ (cả 2 ready)
       ▼
┌──────────────┐
│ STATE_PLAYING │ ← Bắn lần lượt vào map đối thủ
└──────┬───────┘
       │ (game over)
       ▼
┌──────────────┐
│STATE_GAME_OVER│
└──────────────┘
```

### Protocol Messages (Client ↔ Server)

#### Authentication
```
Client → Server: LOGIN:username:password#
Server → Client: LOGIN_OK:username:games:wins:elo:user_id#

Client → Server: REGISTER:username:password#
Server → Client: REGISTER_OK# hoặc REGISTER_FAIL#
```

#### Lobby
```
Client → Server: GET_USERS#
Server → Client: USERS:count:id1,name1,status1,elo1:id2,name2,status2,elo2:#

Client → Server: INVITE:target_user_id#
Server → Target: INVITE_FROM:inviter_id:inviter_name#

Target → Server: ACCEPT_INVITE:inviter_id#
Server → Both:   INVITE_ACCEPTED#
Server → Both:   GAME_START#
```

#### Ship Placement
```
Client → Server: READY#
Server → Client: WAITING_OPPONENT# (nếu đối thủ chưa ready)
Server → Opponent: OPPONENT_READY# (thông báo đối thủ đã ready)

(Khi cả 2 ready)
Server → Both: START_PLAYING#
```

#### Gameplay
```
Client → Server: FIRE:x,y#
Server → Client: HIT#, MISS#, hoặc SUNK#
Server → Client: YOUR_TURN# hoặc OPPONENT_TURN#
```

---

## 2. CẤU TRÚC DỮ LIỆU

### Game Struct (Client)
```c
typedef struct {
    // === SDL Components ===
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;          // Bold font (24pt)
    TTF_Font* font_small;    // Normal font (18pt)
    
    // === State Management ===
    GameState state;         // STATE_LOGIN, STATE_LOBBY, STATE_PLACING_SHIPS, etc.
    int sockfd;              // Socket kết nối server
    pthread_t recv_thread;   // Thread nhận messages từ server
    
    // === Login/Register ===
    InputField username_field;
    InputField password_field;
    int is_register_mode;    // 0=login, 1=register
    char login_message[256];
    
    // === User Info ===
    int my_user_id;
    char my_username[50];
    int my_elo;
    int total_games;
    int wins;
    
    // === Lobby ===
    UserInfo users[MAX_USERS];  // Danh sách users online
    int user_count;
    int selected_user_index;
    int scroll_offset;
    
    // === Invite System ===
    int invited_user_id;        // User mình đang mời
    char invited_username[50];
    int inviter_user_id;        // User đang mời mình
    char inviter_username[50];
    
    // === Game Maps ===
    char own_map[13][13];       // Bản đồ của mình (có tàu)
    char enemy_map[13][13];     // Bản đồ đối thủ (ẩn tàu)
    
    // === Ship Placement ===
    int selected_ship_length;   // 2, 3, hoặc 4
    int selected_ship_id;       // 0, 1, 2 (index trong list)
    int ship_horizontal;        // 1=ngang, 0=dọc
    int ships_placed_count[5];  // ships_placed_count[2], [3], [4]
    int mouse_grid_x;           // Vị trí chuột trên grid
    int mouse_grid_y;
    int preview_valid;          // Preview màu xanh (valid) hay đỏ (invalid)
    
    // === Playing ===
    int is_my_turn;
    char message[256];          // Message hiển thị cho user
    
    int running;                // Main loop flag
} Game;
```

### Map Encoding
```c
char own_map[13][13];
// '-' = Ocean (ô trống)
// '2' = Ship 2 cells
// '3' = Ship 3 cells  
// '4' = Ship 4 cells
// 'x' = Miss (bắn trượt)
// 'o' = Hit (bắn trúng)
// '@' = Sunk (tàu chìm)
```

### Client Struct (Server)
```c
typedef struct {
    int fd;                  // Socket file descriptor
    int user_id;
    char username[50];
    int is_authenticated;
    int in_game;             // 1 nếu đang trong game
    int invited_by;          // user_id của người mời
    int opponent_id;         // user_id của đối thủ (khi đã accept)
    int is_ready;            // 1 khi đã click READY
    pthread_t thread;
} Client;
```

---

## 3. FUNCTIONS CHÍNH

### 3.1 Render Functions

#### `render_placing_ships()`
**Chức năng**: Vẽ màn hình đặt tàu

**Layout**:
```
┌─────────────────────────────────────────────┐
│        DAT TAU - BATTLESHIP                 │
│    Nhan [R] de xoay tau, Click dat          │
├──────────┬──────────────────────────────────┤
│ CHON TAU │       BAN DO CUA BAN             │
│          │                                   │
│ [Lon(4)] │     ┌───────────────┐            │
│ [Vua(3)] │     │  13x13 GRID   │            │
│ [Nho(2)] │     │  with PREVIEW │            │
│          │     └───────────────┘            │
│ HUONG DN │                                   │
│ 1. Chon  │         [READY!]                 │
│ 2. R xoay│                                   │
│ 3. Click │    Da dat: 2/4 tau               │
└──────────┴──────────────────────────────────┘
```

**Code Flow**:
```c
1. Clear screen với COLOR_BG (dark blue)
2. Draw title và instructions (font_small)
3. Draw ship selection list (3 buttons)
   - "Lon (4o) [1/1]"
   - "Vua (3o) [1/1]"
   - "Nho (2o) [2/2]"
4. Draw 13x13 map với cell size 28px
5. Draw ship preview (xanh/đỏ based on valid)
6. Draw READY button (enabled nếu đủ 4 tàu)
7. Draw status message
```

#### `render_playing()`
**Chức năng**: Vẽ màn hình chơi

**Layout**:
```
┌──────────────────────────────────────────────┐
│     BATTLESHIP - TRAN DAU                    │
│          LUOT CUA BAN! (green)               │
├────────────────┬─────────────────────────────┤
│  BAN DO CUA BAN│    BAN DO DOI THU           │
│                │                             │
│ ┌────────────┐ │  ┌────────────┐            │
│ │ 13x13 GRID │ │  │ 13x13 GRID │            │
│ │ Ships shown│ │  │ Ships hidden│            │
│ └────────────┘ │  └────────────┘            │
│                │                             │
│   Ban vao (5,7)... (yellow message)         │
└────────────────┴─────────────────────────────┘
```

### 3.2 Helper Functions

#### `draw_map(map[][], start_x, start_y, is_own_map)`
Vẽ lưới 13x13 với các màu:
- Ocean: Cornflower blue
- Ship: Gray (chỉ hiện nếu is_own_map=1)
- Miss: White với X đen
- Hit: Orange-red với chấm vàng
- Sunk: Dark gray với chấm vàng

#### `check_placement_valid(x, y, length, horizontal)`
Kiểm tra:
- Tàu có nằm trong grid không (0-12)
- Không overlap với tàu khác
- Return 1 (valid) hoặc 0 (invalid)

#### `draw_ship_preview(map_x, map_y)`
Vẽ semi-transparent overlay:
- Green (0,255,0,200) nếu valid
- Red (255,0,0,200) nếu invalid

### 3.3 Event Handlers

#### `handle_placing_ships_click(x, y)`
```c
1. Check ship list clicks (20, 100+i*55)
   → Set selected_ship_length và selected_ship_id
   
2. Check map clicks (200, 100 + 13x28px area)
   → Calculate grid_x, grid_y
   → Place ship if preview_valid
   → Update ships_placed_count[]
   
3. Check READY button (map_x+150, map_y+400)
   → Send "READY#" if all 4 ships placed
```

#### `handle_playing_click(x, y)`
```c
1. Check if my turn
2. Check enemy map clicks (530, 100)
3. Calculate target grid position
4. Send "FIRE:x,y#" to server
```

### 3.4 Network Functions

#### `receive_thread(void* arg)`
**Chức năng**: Thread riêng nhận messages từ server

**Flow**:
```c
while(running) {
    recv(sockfd, buffer) → nhận message
    
    if "LOGIN_OK:" → parse user info, chuyển STATE_LOBBY
    if "USERS:" → parse danh sách users
    if "INVITE_FROM:" → hiện dialog
    if "INVITE_ACCEPTED#" → chuyển STATE_PLACING_SHIPS
    if "START_PLAYING#" → chuyển STATE_PLAYING
    if "WAITING_OPPONENT#" → hiện message chờ
}
```

#### `send_to_server(msg)`
```c
send(sockfd, msg, strlen(msg), 0);
printf("SENT: %s", msg);
```

---

## 4. CÁCH CODE ĐƯỢC NỐI

### 4.1 From client_gui_v2.c → client_gui_login.c

**Files được merge**:

#### Structures
```c
// client_gui_v2.c GameData struct
int selected_ship_length;
int ships_placed_count[5];
char own_map[MAP_SIZE][MAP_SIZE];

// ↓ Copy vào client_gui_login.c Game struct ↓

// Ship placement data (from client_gui_v2.c)
int selected_ship_length;
int selected_ship_id;
int ship_horizontal;
int ships_placed_count[5];
char own_map[MAP_SIZE][MAP_SIZE];
```

#### Functions Copied
```c
// client_gui_v2.c
void draw_text(GameData *game, ...)
void draw_button(GameData *game, ...)
void draw_map(GameData *game, ...)
void draw_ship_preview(GameData *game, ...)
void render_placing_ships(GameData *game)
void render_playing(GameData *game)

// ↓ Adapted to client_gui_login.c ↓

void draw_text(...)           // Dùng global game struct
void draw_button(...)         // Remove game param
void draw_map(...)
void draw_ship_preview(...)
void render_placing_ships()   // No params
void render_playing()
```

#### Event Handling
```c
// client_gui_v2.c
void handle_click_placing_ships(GameData *game, x, y)
void handle_click_playing(GameData *game, x, y)

// ↓ Merged into client_gui_login.c ↓

void handle_placing_ships_click(x, y)  // Dùng global game
void handle_playing_click(x, y)
```

### 4.2 Integration Points

#### Init Game
```c
void init_game() {
    // Original client_gui_login initialization
    SDL_Init(), TTF_Init()
    Create window, renderer, fonts
    
    // NEW: Initialize ship data from client_gui_v2
    game.selected_ship_length = 0;
    game.ships_placed_count[2] = 0;
    game.ships_placed_count[3] = 0;
    game.ships_placed_count[4] = 0;
    game.ship_horizontal = 1;
}
```

#### Main Render Loop
```c
void render() {
    pthread_mutex_lock(&game_lock);
    
    // Original states
    if(state == STATE_LOGIN) render_login_screen();
    if(state == STATE_LOBBY) render_lobby_screen();
    
    // NEW: Integrated ship placement
    if(state == STATE_PLACING_SHIPS) render_placing_ships();
    
    // NEW: Integrated gameplay
    if(state == STATE_PLAYING) render_playing();
    
    pthread_mutex_unlock(&game_lock);
    SDL_RenderPresent(renderer);
}
```

#### Event Handling
```c
void handle_events() {
    SDL_PollEvent(&e);
    
    // Original login/lobby clicks
    if(state == STATE_LOGIN) handle_login_click(x, y);
    if(state == STATE_LOBBY) handle_lobby_click(x, y);
    
    // NEW: Ship placement events
    if(state == STATE_PLACING_SHIPS) {
        if(mouse_motion) → update preview
        if(mouse_click) → handle_placing_ships_click()
        if(key_R) → toggle ship_horizontal
    }
    
    // NEW: Gameplay events  
    if(state == STATE_PLAYING) {
        if(mouse_click) → handle_playing_click()
    }
}
```

---

## 5. SERVER LOGIC

### Game Room Management

```c
// Khi Accept Invite
client->opponent_id = inviter_id;
inviter->opponent_id = client_id;
client->is_ready = 0;
inviter->is_ready = 0;
send GAME_START# → cả 2 clients

// Khi READY
client->is_ready = 1;
if(opponent->is_ready == 1) {
    send START_PLAYING# → cả 2
} else {
    send WAITING_OPPONENT# → client
    send OPPONENT_READY# → opponent
}
```

---

## 6. DEBUGGING TIPS

### Client Debug
```bash
# Xem messages nhận được
grep "RECEIVED:" output.log

# Test ship placement
1. Click ship button → check selected_ship_length
2. Hover map → check mouse_grid_x, mouse_grid_y
3. Click map → check ships_placed_count[]
```

### Server Debug
```bash
# Xem commands từ clients
grep "RECEIVED from fd=" server.log

# Track game state
grep "is READY\|Game starting" server.log
```

### Common Issues

**Ships không đặt được**:
- Check `preview_valid` = 1?
- Check `selected_ship_length` > 0?
- Check map coordinates đúng không?

**READY không work**:
- Check server nhận được READY#?
- Check opponent_id đã set chưa?
- Check cả 2 players đều is_ready = 1?

**Map không update**:
- Check receive_thread đang chạy?
- Check pthread_mutex_lock đúng chỗ?
- Check server gửi STATE messages?

---

## 7. TIMELINE INTEGRATION

### Phase 1: Structure Setup ✅
- Add ship fields to Game struct
- Copy color definitions
- Initialize ship data in init_game()

### Phase 2: Helper Functions ✅
- draw_text(), draw_button()
- draw_map() with cell rendering
- check_placement_valid()
- draw_ship_preview()

### Phase 3: Render Screens ✅
- render_placing_ships() with layout
- render_playing() with dual maps

### Phase 4: Event Handling ✅
- Mouse motion for preview
- Click handlers for ship placement
- Click handlers for firing

### Phase 5: Network Protocol ✅
- Server READY handler
- Client START_PLAYING receiver
- Game state transitions

### Phase 6: UI Polish ✅
- Fix layout overlapping
- Change to normal font
- Add status counter
- Add instructions

---

## 8. TESTING CHECKLIST

### Ship Placement
- [ ] Click ship buttons → selected highlight
- [ ] Hover map → green/red preview
- [ ] Press R → ship rotates
- [ ] Click map → ship placed, count updates
- [ ] 4 ships → READY enabled
- [ ] Click READY → message "Cho doi thu..."

### Game Start
- [ ] Both READY → receive START_PLAYING#
- [ ] State changes to STATE_PLAYING
- [ ] Dual maps shown
- [ ] Turn indicator correct

### Gameplay
- [ ] My turn → can click enemy map
- [ ] Not my turn → clicks ignored
- [ ] Fire → FIRE:x,y# sent
- [ ] Hit/Miss → map updates

---

**Kết luận**: Code được nối bằng cách copy structures, functions và event handlers từ client_gui_v2.c vào client_gui_login.c, sau đó adapt để dùng global `game` struct thay vì pass parameters. Server được extend với opponent tracking và READY handling để support multiplayer flow.

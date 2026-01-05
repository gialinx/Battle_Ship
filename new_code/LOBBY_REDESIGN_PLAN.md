# Kế hoạch thiết kế lại Lobby Screen - Feature-Rich Version

## Tổng quan thiết kế

```
┌─────────────────────────────────────────────────────────────────┐
│  [LOGO]          [FIND MATCH]                   [AVA] Username  │
│                                                         ELO: XXX │
├──────────────────────┬──────────────────────────────────────────┤
│                      │ ┌──────────────────────────────────────┐ │
│                      │ │ Search: [_____________]         [X]  │ │
│                      │ ├──────────────────────────────────────┤ │
│                      │ │ • Player1      ELO: 1200    [INVITE] │ │
│   MAIN CONTENT       │ │ • Player2      ELO: 1150    [INVITE] │ │
│   (Leaderboard/      │ │ • Player3      ELO: 1100    [INVITE] │ │
│    Stats/History)    │ │ • Player4      ELO: 1050    [INVITE] │ │
│                      │ │ ...                                   │ │
│                      │ │                                       │ │
│                      │ └──────────────────────────────────────┘ │
│                      │                                           │
└──────────────────────┴──────────────────────────────────────────┘
```

## Cấu trúc Layout (1000x700px)

### 1. Header Bar (1000x80px)
- **Logo (0-150px)**: Logo game ở góc trái
- **Find Match Button (200-400px)**: Nút tìm trận lớn, nổi bật
- **User Info (750-1000px)**: Avatar + Username + ELO

### 2. Main Area (700x620px)
- **Left Panel (0-700px)**: Tabs (Leaderboard / Stats / History)
- **Right Panel (700-1000px)**: Player List với search

---

## Phần 1: Cấu trúc dữ liệu mới

### GameData additions (game_data.h)
```c
// Lobby UI state
typedef enum {
    LOBBY_TAB_LEADERBOARD,
    LOBBY_TAB_STATS,
    LOBBY_TAB_HISTORY
} LobbyTab;

// Match history entry
typedef struct {
    int match_id;
    char opponent_name[50];
    int my_elo_before;
    int my_elo_after;
    int elo_change;
    int result;  // 1=win, 0=lose
    char date[50];
} MatchHistoryEntry;

// Leaderboard entry
typedef struct {
    int rank;
    char username[50];
    int elo_rating;
    int total_games;
    int wins;
    float win_rate;
} LeaderboardEntry;

// Trong GameData:
LobbyTab active_lobby_tab;
InputField player_search_field;
int matchmaking_active;  // 1 nếu đang trong hàng chờ

LeaderboardEntry leaderboard[20];
int leaderboard_count;

MatchHistoryEntry match_history[20];
int match_history_count;

// Personal stats
int losses;
float win_rate;
int rank;  // Hạng hiện tại
```

---

## Phần 2: Server Changes (Backend)

### 2.1. Server - Matchmaking System
**File**: `server/src/server_lobby.c`

**New features**:
- Queue data structure cho matchmaking
- ELO-based matching algorithm (+/- 100 ELO)
- Auto-match sau 10s nếu không tìm được gần

**Protocol**:
```
Client → Server: FIND_MATCH#
Server → Client: MATCHMAKING_STARTED#
Server → Client: MATCH_FOUND:opponent_name#  (khi tìm được)
Server → Client: MATCHMAKING_TIMEOUT#  (sau 30s không tìm được)

Client → Server: CANCEL_MATCHMAKING#
Server → Client: MATCHMAKING_CANCELLED#
```

### 2.2. Server - Leaderboard API
**File**: `server/src/database.c`

**New function**:
```c
int db_get_leaderboard(LeaderboardEntry* entries, int limit);
```

**Protocol**:
```
Client → Server: GET_LEADERBOARD#
Server → Client: LEADERBOARD:rank:username:elo:games:wins#rank:username...#
```

### 2.3. Server - Match History API
**File**: `server/src/database.c`

**New function**:
```c
int db_get_match_history(int user_id, MatchHistoryEntry* entries, int limit);
```

**Protocol**:
```
Client → Server: GET_HISTORY#
Server → Client: HISTORY:match_id:opponent:elo_before:elo_after:result:date#...#
```

---

## Phần 3: Client Changes (Frontend)

### 3.1. Lobby Screen - Render
**File**: `client/src/ui/screens/lobby_screen.c`

**New render functions**:
```c
void lobby_render_header(SDL_Renderer* renderer, GameData* game);
void lobby_render_logo(SDL_Renderer* renderer, GameData* game);
void lobby_render_find_match_button(SDL_Renderer* renderer, GameData* game);
void lobby_render_user_info(SDL_Renderer* renderer, GameData* game);

void lobby_render_main_panel(SDL_Renderer* renderer, GameData* game);
void lobby_render_tabs(SDL_Renderer* renderer, GameData* game);
void lobby_render_leaderboard_tab(SDL_Renderer* renderer, GameData* game);
void lobby_render_stats_tab(SDL_Renderer* renderer, GameData* game);
void lobby_render_history_tab(SDL_Renderer* renderer, GameData* game);

void lobby_render_player_list(SDL_Renderer* renderer, GameData* game);
void lobby_render_search_box(SDL_Renderer* renderer, GameData* game);

void lobby_render_matchmaking_dialog(SDL_Renderer* renderer, GameData* game);
```

**Layout details**:
- Header: 0-80px height
- Main tabs: 80-700px height, 0-700px width
- Player list: 80-700px height, 700-1000px width

### 3.2. Lobby Screen - Click Handlers
**File**: `client/src/ui/screens/lobby_screen.c`

```c
void lobby_handle_header_click(GameData* game, int x, int y);
void lobby_handle_find_match_click(GameData* game);
void lobby_handle_avatar_click(GameData* game);
void lobby_handle_tab_click(GameData* game, int x, int y);
void lobby_handle_player_list_click(GameData* game, int x, int y);
void lobby_handle_search_input(GameData* game, SDL_Event* e);
```

### 3.3. Protocol Parsing
**File**: `client/src/network/protocol.c`

**New parsers**:
```c
int parse_leaderboard(GameData* game, const char* msg);
int parse_match_history(GameData* game, const char* msg);
int parse_matchmaking_status(GameData* game, const char* msg);
```

---

## Phần 4: Assets

### Ảnh cần có:
1. **logo.png** (150x60px): Logo game cho header
2. **default_avatar.png** (60x60px): Avatar mặc định
3. **search_icon.png** (20x20px): Icon tìm kiếm
4. **tab_background.png**: Background cho tabs

### Placeholder nếu chưa có ảnh:
- Logo: Text "BATTLESHIP" màu cyan
- Avatar: Hình tròn màu xanh với chữ cái đầu của username
- Icons: Dùng text ("🔍", "📊", etc.)

---

## Phần 5: Implementation Plan

### Phase 1: Basic Layout ✅
1. Tạo layout mới với header/main/player-list
2. Di chuyển player list sang bên phải
3. Thêm search box (chưa hoạt động)
4. Thêm logo placeholder
5. Thêm user info ở góc phải

### Phase 2: Tabs System ✅
1. Tạo enum LobbyTab
2. Render 3 tabs: Leaderboard / Stats / History
3. Click để chuyển tab
4. Active tab highlight

### Phase 3: Leaderboard Tab ✅
1. Server: Implement `db_get_leaderboard()`
2. Server: Handle `GET_LEADERBOARD` protocol
3. Client: Request leaderboard khi vào lobby
4. Client: Parse và hiển thị top 10
5. Format: Rank | Username | ELO | Games | Win Rate

### Phase 4: Stats Tab ✅
1. Tính toán win rate từ wins/total_games
2. Hiển thị:
   - Total Games
   - Wins / Losses
   - Win Rate (%)
   - Current Rank
   - ELO progression graph (simple bar)

### Phase 5: History Tab ✅
1. Server: Implement `db_get_match_history()`
2. Server: Handle `GET_HISTORY` protocol
3. Client: Request history khi vào lobby
4. Client: Parse và hiển thị 10 trận gần nhất
5. Format: Date | Opponent | Result | ELO Change

### Phase 6: Matchmaking System ✅
1. Server: Tạo matchmaking queue
2. Server: Implement ELO-based matching
3. Server: Handle FIND_MATCH / CANCEL_MATCHMAKING
4. Client: Find Match button
5. Client: Matchmaking dialog (searching animation)
6. Client: Cancel button trong dialog

### Phase 7: Player Search ✅
1. Filter player list theo search text
2. Real-time search khi gõ
3. Clear button ("X")

### Phase 8: Profile Screen (Future)
1. Click avatar → STATE_PROFILE
2. Profile screen: Full stats, logout button
3. Back button → STATE_LOBBY

---

## Phần 6: Code Changes Summary

### Files to CREATE:
- `server/src/matchmaking.c` + `.h`: Matchmaking logic
- `client/src/ui/screens/profile_screen.c` + `.h`: Profile screen (Phase 8)

### Files to MODIFY:
1. `server/src/database.c`: Add leaderboard + history queries
2. `server/src/database.h`: Add function declarations
3. `server/src/server_lobby.c`: Add matchmaking + leaderboard + history handlers
4. `client/src/core/game_data.h`: Add new structs + fields
5. `client/src/ui/screens/lobby_screen.c`: Complete rewrite
6. `client/src/network/protocol.c`: Add new parsers
7. `client/src/network/protocol.h`: Add function declarations

---

## Phần 7: Testing Checklist

### Lobby Render:
- [ ] Logo hiển thị đúng vị trí
- [ ] Find Match button nổi bật
- [ ] User info (avatar + name + ELO) ở góc phải
- [ ] 3 tabs render đúng
- [ ] Player list ở bên phải
- [ ] Search box hoạt động

### Leaderboard:
- [ ] Request thành công
- [ ] Parse data đúng
- [ ] Hiển thị top 10
- [ ] Format đẹp, dễ đọc

### Stats:
- [ ] Win rate tính đúng
- [ ] Hiển thị đầy đủ thông tin
- [ ] Layout gọn gàng

### History:
- [ ] Request thành công
- [ ] Hiển thị 10 trận gần nhất
- [ ] Format: Date, Opponent, Result, ELO change

### Matchmaking:
- [ ] Find Match button click → vào queue
- [ ] Dialog hiển thị "Searching..."
- [ ] Tìm được đối thủ → notification
- [ ] Cancel button hoạt động
- [ ] Timeout sau 30s

### Player Search:
- [ ] Filter theo text input
- [ ] Real-time update
- [ ] Clear button

---

## Phần 8: Priorities

### Must Have (MVP):
1. ✅ Layout mới với header/main/player-list
2. ✅ Tabs system (Leaderboard / Stats / History)
3. ✅ Leaderboard tab (server + client)
4. ✅ Stats tab (client only, tính từ data có sẵn)
5. ✅ Find Match button + Matchmaking system
6. ✅ Player search

### Nice to Have:
1. ⏳ History tab (server + client)
2. ⏳ Profile screen
3. ⏳ Avatar upload
4. ⏳ Animations (fade in/out tabs)
5. ⏳ Sound effects (button click, match found)

### Future:
1. Friends list
2. Chat system
3. Replay system
4. Achievements

---

## Estimated Work:
- **Phase 1-2 (Layout + Tabs)**: 30 minutes
- **Phase 3 (Leaderboard)**: 45 minutes
- **Phase 4 (Stats)**: 15 minutes
- **Phase 5 (History)**: 30 minutes
- **Phase 6 (Matchmaking)**: 60 minutes
- **Phase 7 (Search)**: 15 minutes

**Total**: ~3 hours

---

## Questions for User:

1. **Find Match behavior**:
   - Tự động accept khi tìm được match?
   - Hay hiện popup confirm "Match found with Player X. Accept?"

2. **Leaderboard**:
   - Top 10 hay top 20?
   - Highlight current user nếu có trong top?

3. **Assets**:
   - Bạn có sẵn logo/avatar images chưa?
   - Hay dùng placeholder text tạm?

4. **Profile screen**:
   - Làm ngay hay để sau khi lobby xong?

5. **Matchmaking timeout**:
   - 30s có OK không?
   - Nếu timeout, có tự động expand range (+/- 200 ELO) và retry không?

# INVITATION FLOW DOCUMENTATION

## Tổng quan
Hệ thống invitation cho phép người chơi mời nhau chơi game theo luồng clean và rõ ràng.

## File Structure
```
client/src/ui/screens/
├── invitation_screen.c    # Screen riêng cho invitation (NEW)
├── invitation_screen.h    # Header file
├── lobby_screen.c         # Hiển thị danh sách player online + ELO
└── ...
```

## Flow Chi Tiết

### 1. LOBBY SCREEN - Danh sách Player Online
**File:** `lobby_screen.c`

**Hiển thị:**
- Danh sách tất cả player đang online
- Mỗi player hiển thị:
  - Tên username
  - Điểm ELO
- Search box để tìm kiếm player
- Click vào player để gửi invitation

**Code:**
```c
// Render player với ELO
render_text(renderer, game->font_small, u->username, x, y, white);
snprintf(elo_text, sizeof(elo_text), "ELO: %d", u->elo_rating);
render_text(renderer, game->font_small, elo_text, x, y+20, gray);
```

### 2. STATE_SENDING_INVITE - Đang gửi lời mời
**File:** `invitation_screen.c`

**Khi nào:** Ngay khi click vào player trong lobby

**Hiển thị:**
- Dialog box với tiêu đề "Sending Invitation..."
- Message: "Sending invite to [username]..."
- Animation spinner (|, /, -, \)
- "Please wait [spinner]"

**Server response:**
- `INVITE_SENT#` → chuyển sang STATE_WAITING_INVITE
- `INVITE_FAIL:reason#` → về STATE_LOBBY với error message

### 3. STATE_WAITING_INVITE - Chờ phản hồi
**File:** `invitation_screen.c`

**Người mời thấy:**
- Dialog box với tiêu đề "Invitation Sent" (màu xanh lá)
- Message: "Invitation sent to [username]."
- Sub-message: "Waiting for response..." (màu vàng)
- Button **CANCEL** (màu đỏ, 300x60px)

**User actions:**
- Click **CANCEL** → gửi `CANCEL_INVITE#` → về STATE_LOBBY
- Đối thủ accept → nhận `INVITE_ACCEPTED#` → STATE_PLACING_SHIPS
- Đối thủ decline → nhận `INVITE_DECLINED:username#` → STATE_LOBBY + show decline dialog

### 4. STATE_RECEIVED_INVITE - Nhận lời mời
**File:** `invitation_screen.c`

**Người được mời thấy:**
- Dialog box với tiêu đề "Game Invitation" (màu cyan)
- Message: "[Username] has sent you an invitation to play"
- 2 buttons:
  - **ACCEPT** (màu xanh lá, 200x60px)
  - **DECLINE** (màu đỏ, 200x60px)

**User actions:**
- Click **ACCEPT** → gửi `ACCEPT_INVITE:inviter_user_id#` → STATE_PLACING_SHIPS
- Click **DECLINE** → gửi `DECLINE_INVITE:inviter_user_id#` → STATE_LOBBY

### 5. DECLINE NOTIFICATION - Thông báo bị từ chối
**File:** `invitation_screen.c`

**Khi nào:** Người mời nhận được `INVITE_DECLINED:username#`

**Hiển thị:**
- Dialog box với tiêu đề "Invitation Declined" (màu đỏ)
- Message: "[Username] declined the invitation!"
- Button **X** (60x60px, màu đỏ) để đóng dialog

**User actions:**
- Click **X** → xóa message → về STATE_LOBBY bình thường

## Server Protocol

### Client → Server
```
INVITE:user_id#              // Gửi lời mời
ACCEPT_INVITE:inviter_id#    // Chấp nhận lời mời
DECLINE_INVITE:inviter_id#   // Từ chối lời mời
CANCEL_INVITE#               // Hủy lời mời đã gửi
```

### Server → Client
```
INVITE_SENT#                 // Gửi thành công, chờ response
INVITE_FAIL:reason#          // Gửi thất bại
INVITE_FROM:user_id:username# // Nhận lời mời từ ai
INVITE_ACCEPTED#             // Đối thủ chấp nhận
INVITE_DECLINED:username#    // Đối thủ từ chối
INVITE_CANCELLED#            // Lời mời bị hủy
INVITE_CANCEL_OK#            // Hủy thành công
```

## Server Logic
**File:** `server/src/server_lobby.c`

### handle_invite()
1. Kiểm tra user_id có authenticated không
2. Kiểm tra target user có online không
3. Kiểm tra target user có đang chơi không
4. Gửi `INVITE_FROM:user_id:username#` cho target
5. Gửi `INVITE_SENT#` cho người mời
6. Lưu state: `target->invited_by = inviter_id`

### handle_accept_invite()
1. Tìm inviter từ inviter_id
2. Gửi `INVITE_ACCEPTED#` cho cả 2
3. Set `in_game = 1` cho cả 2
4. Set opponent_id cho cả 2
5. Khởi tạo game data (maps, ships)
6. Gửi `GAME_START#` cho cả 2

### handle_decline_invite()
1. Tìm inviter từ inviter_id
2. Gửi `INVITE_DECLINED:username#` cho inviter
3. Gửi `DECLINE_OK#` cho người decline
4. Clear invite state

### handle_cancel_invite()
1. Tìm người được mời (invited_by == current_user_id)
2. Gửi `INVITE_CANCELLED#` cho người được mời
3. Gửi `INVITE_CANCEL_OK#` cho người cancel
4. Clear invite state

## State Transitions

```
LOBBY
  ├─ Click player ──→ SENDING_INVITE
  │                        │
  │                        ├─ Success ──→ WAITING_INVITE
  │                        │                   │
  │                        │                   ├─ CANCEL ──→ LOBBY
  │                        │                   ├─ ACCEPTED ──→ PLACING_SHIPS
  │                        │                   └─ DECLINED ──→ LOBBY (with decline dialog)
  │                        │
  │                        └─ Fail ──→ LOBBY (with error)
  │
  └─ Receive invite ──→ RECEIVED_INVITE
                             │
                             ├─ ACCEPT ──→ PLACING_SHIPS
                             └─ DECLINE ──→ LOBBY

PLACING_SHIPS (cả 2 player)
  └─ Both READY ──→ PLAYING

PLAYING
  └─ Game ends ──→ GAME_OVER ──→ LOBBY
```

## UI Design

### Dialog Box Dimensions
- Width: 600px
- Height: 400px
- Position: Center screen (200, 150)
- Background: Dark blue (40, 50, 70)
- Border: Light blue (100, 150, 200), 3px

### Button Sizes
- CANCEL button: 300x60px
- ACCEPT/DECLINE buttons: 200x60px each
- Close X button: 60x60px

### Colors
- Title colors:
  - Sending: Cyan (0, 200, 255)
  - Sent: Green (0, 200, 0)
  - Received: Cyan (0, 200, 255)
  - Declined: Red (200, 0, 0)
- Text: White (255, 255, 255)
- Waiting text: Yellow (255, 200, 0)
- Buttons: Green/Red based on action

## Testing Checklist

- [ ] Player list hiển thị đúng username + ELO
- [ ] Click player → hiện "Sending..." với spinner
- [ ] Server nhận INVITE → chuyển sang "Waiting for response"
- [ ] Button Cancel hoạt động đúng
- [ ] Người được mời nhận được dialog với 2 button
- [ ] Accept → cả 2 chuyển sang PLACING_SHIPS
- [ ] Decline → người được mời về lobby, người mời thấy decline notification
- [ ] Click X trên decline notification → về lobby
- [ ] Cancel invite → người được mời thấy "Invitation cancelled"

## Notes

- Code clean, mỗi screen 1 file riêng
- invitation_screen.c xử lý toàn bộ UI invitation
- lobby_screen.c chỉ xử lý player list
- protocol.c xử lý parse server messages
- server_lobby.c xử lý game logic

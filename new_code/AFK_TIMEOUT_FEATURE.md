# ✅ CHỨC NĂNG AFK TIMEOUT ĐÃ CÓ SẴN

## Tóm tắt

Hệ thống **ĐÃ CÓ** chức năng AFK (Away From Keyboard) detection hoạt động **CHÍNH XÁC** theo yêu cầu của bạn!

## Cách hoạt động

### 1. Server - AFK Detection Thread

**File**: `new_code/server/src/server_lobby.c` (dòng 1991-2045)

- **Thread chạy nền**: Kiểm tra mỗi 30 giây
- **Chỉ kiểm tra**: Người chơi đang trong game (`in_game = true`)
- **Tracking**: Theo dõi `last_activity_time` của mỗi client

### 2. Timeline AFK

```
┌─────────────────────────────────────────────────────────────┐
│  0'         3'                            5'                │
│  │          │                             │                 │
│  │          ▼                             ▼                 │
│  │     AFK WARNING                  AUTO FORFEIT            │
│  │     (hiện popup)                 (tự động thua)          │
│  │          │                             │                 │
│  │          └──────── 2 phút ─────────────┘                 │
│  │                                                           │
│  └── Người chơi hoạt động bình thường                       │
└─────────────────────────────────────────────────────────────┘
```

#### ⏱️ Thời gian cụ thể:

1. **0 - 3 phút**: Chơi bình thường
2. **3 phút**: 
   - Server gửi `AFK_WARNING#`
   - Client hiện popup cảnh báo
   - Text: "Are you still there? You have been inactive for 3 minutes."
3. **3 - 5 phút** (2 phút để phản hồi):
   - Chờ người chơi click "I'M HERE!"
   - Nếu click → Reset timer, tiếp tục chơi
   - Nếu không → Timeout
4. **5 phút**: 
   - Server tự động gọi `handle_forfeit()`
   - Người chơi thua, đối thủ thắng
   - Cập nhật ELO và lưu database

### 3. Cập nhật Last Activity

Server cập nhật `last_activity_time` khi:
- Client gửi **BẤT KỲ** lệnh nào
- Đặt tàu (PLACE)
- Bắn (FIRE)
- Chấp nhận lời mời
- Gửi tin nhắn
- Phản hồi AFK warning

### 4. Client - AFK Warning Screen

**File**: `new_code/client/src/ui/screens/afk_warning_screen.c`

**Màn hình popup**:
```
┌───────────────────────────────────────┐
│      ⚠ AFK WARNING ⚠                 │
│                                       │
│    Are you still there?               │
│    You have been inactive for 3       │
│    minutes. If you don't respond      │
│    in 2 minutes, you will forfeit     │
│    the match.                         │
│                                       │
│         [  I'M HERE!  ]               │
│                                       │
└───────────────────────────────────────┘
```

**Khi click "I'M HERE!"**:
- Gửi `AFK_RESPONSE#` về server
- Server reset `afk_warned = 0`
- Server reset timer (cập nhật `last_activity_time`)
- Popup đóng, game tiếp tục

### 5. Áp dụng cho tất cả game states

AFK detection hoạt động trong:
- ✅ **Đặt tàu** (STATE_PLACING_SHIPS)
- ✅ **Chơi game** (STATE_PLAYING)
- ❌ Không check khi ở lobby/menu

## Code Flow

### Server Side
```c
// AFK Detection Thread (every 30s)
for each client in game:
    idle_time = now - last_activity_time
    
    if idle_time >= 300s && afk_warned:
        → AUTO FORFEIT (người chơi thua)
        
    if idle_time >= 180s && !afk_warned:
        → Send AFK_WARNING#
        → Set afk_warned = 1
```

### Client Side
```c
// Receive AFK_WARNING#
game->afk_warning_visible = 1

// User clicks "I'M HERE!"
send("AFK_RESPONSE#")
game->afk_warning_visible = 0
```

## Kiểm tra log

### Server logs
```bash
[AFK] username has been idle for 180 seconds - sending warning
[AFK] username responded to AFK warning
# hoặc
[AFK] username has been AFK for 300 seconds - auto forfeiting
[FORFEIT] username surrendered to opponent (Match ID=X, ELO change: -25 vs +25)
```

### Client logs
```bash
Server: AFK_WARNING#
CLIENT: Responded to AFK warning
```

## Test thử

### Test 1: Cảnh báo AFK

1. Vào game (placing ships hoặc playing)
2. **Không làm gì** trong 3 phút
3. ✅ **Kết quả**: Popup AFK warning xuất hiện
4. Click "I'M HERE!"
5. ✅ **Kết quả**: Popup đóng, tiếp tục chơi

### Test 2: Auto forfeit

1. Vào game
2. **Không làm gì** trong 3 phút → popup xuất hiện
3. **Tiếp tục không làm gì** thêm 2 phút (tổng 5 phút)
4. ✅ **Kết quả**: 
   - Tự động thua
   - Đối thủ thắng
   - ELO cập nhật
   - Match lưu vào database

### Test 3: Đặt tàu AFK

1. Vào màn hình đặt tàu
2. Đặt 1-2 tàu rồi dừng
3. Đợi 3 phút → popup
4. Đợi thêm 2 phút → forfeit

## So sánh với Disconnect

| Tình huống | Thời gian | Kết quả | Database marker |
|-----------|-----------|---------|-----------------|
| **AFK** | 5 phút không hoạt động | Auto forfeit | "FORFEIT" |
| **Disconnect** | Ngay lập tức | Auto forfeit | "DISCONNECT" |
| **Surrender** | Người chơi chủ động | Forfeit | "FORFEIT" |

## Cấu hình

Nếu muốn thay đổi thời gian, chỉnh trong `server_lobby.c` (dòng 1995-1996):

```c
const int AFK_WARNING_TIMEOUT = 180;  // 3 phút = 180 giây
const int AFK_FORFEIT_TIMEOUT = 300;  // 5 phút = 300 giây
```

Ví dụ muốn 2 phút warning, 4 phút forfeit:
```c
const int AFK_WARNING_TIMEOUT = 120;  // 2 phút
const int AFK_FORFEIT_TIMEOUT = 240;  // 4 phút
```

## Kết luận

✅ **Chức năng đã có sẵn và hoạt động đúng yêu cầu**:
- ✅ 3 phút không hoạt động → Cảnh báo
- ✅ Hiện popup hỏi người chơi
- ✅ 2 phút (tổng 5 phút) không trả lời → Auto forfeit
- ✅ Áp dụng cho placing ships và playing game
- ✅ Thread chạy nền, không ảnh hưởng performance
- ✅ Tích hợp với ELO và database

**Không cần code thêm gì!** Chỉ cần chạy server và test thôi! 🎮

# BUG FIX: READY Button Error

## Vấn đề
Khi cả 2 player đã đặt đủ 4/4 tàu và click READY, server vẫn trả về lỗi:
```
ERROR:Place all ships first#
```

## Nguyên nhân
Client chỉ cập nhật bản đồ local (`own_map` và `ships_placed_count`) nhưng **KHÔNG GỬI lệnh `PLACE` đến server**.

Server không biết client đã đặt tàu gì, nên khi nhận `READY#`, server kiểm tra `ship_count` vẫn bằng 0 → trả về lỗi.

## Giải pháp
Thêm code gửi lệnh `PLACE` đến server khi player click đặt tàu:

### File đã sửa: `placing_ships_screen.c`

**Before:**
```c
// Click on map to place ship
if(game->selected_ship_length > 0 && game->preview_valid) {
    // Chỉ cập nhật local map
    for(int i = 0; i < length; i++) {
        game->own_map[ty][tx] = '0' + length;
    }
    game->ships_placed_count[length]++;
}
```

**After:**
```c
// Click on map to place ship
if(game->selected_ship_length > 0 && game->preview_valid) {
    // GỬI LỆNH PLACE ĐẾN SERVER
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "PLACE:%d,%d,%d,%c#", 
             length, grid_x + 1, grid_y + 1, horizontal ? 'H' : 'V');
    send(game->sockfd, buffer, strlen(buffer), 0);
    printf("CLIENT: Sent PLACE command: %s\n", buffer);
    
    // Cập nhật local map
    for(int i = 0; i < length; i++) {
        game->own_map[ty][tx] = '0' + length;
    }
    game->ships_placed_count[length]++;
}
```

## Protocol Flow

### Đúng (sau khi fix):
```
1. Client click đặt tàu
   → Client: PLACE:4,5,5,H#
   → Server: Lưu vào client->ships[ship_count++]
   → Server: PLACE_OK:#
   → Server: STATE:... (cập nhật map)

2. Client đặt đủ 4 tàu
   → Client click READY
   → Client: READY#
   → Server: check_ready_ships(client) = TRUE (ship_count = 4)
   → Server: READY_OK:#

3. Cả 2 READY
   → Server: START_PLAYING#
   → Game bắt đầu!
```

### Sai (trước khi fix):
```
1. Client click đặt tàu
   ✗ Client KHÔNG gửi PLACE đến server
   ✗ Server ship_count vẫn = 0

2. Client click READY
   → Client: READY#
   → Server: check_ready_ships(client) = FALSE (ship_count = 0)
   → Server: ERROR:Place all ships first#
```

## Testing
1. Start server: `./run_server.sh`
2. Login 2 clients: Alice và Bob
3. Alice invite Bob → Accept
4. Cả 2 đặt tàu:
   - 1 tàu 4 ô
   - 1 tàu 3 ô  
   - 2 tàu 2 ô
5. Click READY
6. ✅ Game bắt đầu thành công!

## Files Changed
- `/new_code/client/src/ui/screens/placing_ships_screen.c`
  - Added: Send PLACE command to server
  - Added: Include `<sys/socket.h>` and `<unistd.h>`

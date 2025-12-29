# Ví dụ cụ thể: Thêm âm thanh và ảnh vào game

File này hướng dẫn chi tiết cách thêm assets vào từng màn hình của game.

## 1. Chuẩn bị assets

Giả sử bạn có các file sau:

### Ảnh (assets/images/):
- `explosion.png` - Hiệu ứng nổ khi trúng đích
- `water_splash.png` - Hiệu ứng nước văng khi miss
- `ship_icon.png` - Icon tàu chiến
- `background.jpg` - Ảnh nền

### Âm thanh (assets/sounds/):
- `shot.wav` - Tiếng bắn
- `hit.wav` - Tiếng trúng đích
- `miss.wav` - Tiếng trượt
- `ship_sink.wav` - Tiếng tàu chìm
- `victory.wav` - Nhạc thắng
- `defeat.wav` - Nhạc thua
- `background.mp3` - Nhạc nền

---

## 2. Load assets khi khởi động

Mở file: **`src/client/client_gui_complete.c`**

Tìm đoạn code có comment `// TODO: Load ảnh và âm thanh ở đây` (dòng ~85)

Thay thế bằng:

```c
// Load các ảnh
fprintf(stderr, "DEBUG: Loading images...\n");
assets_load_image(&game.assets, game.renderer, "explosion.png");
assets_load_image(&game.assets, game.renderer, "water_splash.png");
assets_load_image(&game.assets, game.renderer, "ship_icon.png");
assets_load_image(&game.assets, game.renderer, "background.jpg");

// Load các âm thanh
fprintf(stderr, "DEBUG: Loading sounds...\n");
assets_load_sound(&game.assets, "shot.wav");
assets_load_sound(&game.assets, "hit.wav");
assets_load_sound(&game.assets, "miss.wav");
assets_load_sound(&game.assets, "ship_sink.wav");
assets_load_sound(&game.assets, "victory.wav");
assets_load_sound(&game.assets, "defeat.wav");

// Load nhạc nền
assets_load_music(&game.assets, "background.mp3");

fprintf(stderr, "DEBUG: Assets loaded successfully!\n");
```

---

## 3. Sử dụng trong màn hình chơi

### 3.1. Thêm âm thanh khi bắn

Mở file: **`src/ui/screens/playing_screen.c`**

Trong hàm `playing_screen_handle_click()`, tìm đoạn xử lý khi click vào enemy map (dòng ~96):

```c
// Will send FIRE command (handled by main client)
snprintf(game->message, sizeof(game->message), "Firing at (%d,%d)...", col + 1, row + 1);

// THÊM: Phát tiếng bắn
assets_play_sound(&game->assets, "shot.wav", 0);
```

### 3.2. Thêm hiệu ứng khi trúng/trượt

Mở file: **`src/network/protocol.c`**

#### Khi HIT (trúng đích):
Tìm dòng xử lý `HIT_CONTINUE:` (dòng ~260):

```c
if(strcmp(msg, "HIT_CONTINUE:") == 0) {
    snprintf(game->message, sizeof(game->message), "HIT! Fire again!");

    // THÊM: Phát tiếng trúng
    assets_play_sound(&game->assets, "hit.wav", 0);

    return 1;
}
```

#### Khi MISS (trượt):
Tìm dòng xử lý `WAIT_YOUR_TURN:` (dòng ~254), thêm logic kiểm tra:

```c
if(strcmp(msg, "WAIT_YOUR_TURN:") == 0) {
    game->is_my_turn = 0;
    game->state = STATE_PLAYING;

    // Nếu không trúng thì phát tiếng miss
    assets_play_sound(&game->assets, "miss.wav", 0);

    snprintf(game->message, sizeof(game->message), "Opponent is firing...");
    return 1;
}
```

### 3.3. Vẽ hiệu ứng nổ lên màn hình

Trong **`src/ui/screens/playing_screen.c`**, hàm `playing_screen_render()`:

Sau khi vẽ map (dòng ~39), thêm:

```c
// Vẽ các hiệu ứng trên enemy map
for(int row = 0; row < MAP_SIZE; row++) {
    for(int col = 0; col < MAP_SIZE; col++) {
        char cell = game->enemy_map[row][col];
        int x = ENEMY_MAP_X + col * CELL_SIZE;
        int y = ENEMY_MAP_Y + row * CELL_SIZE;

        // Nếu là ô trúng đích (x) thì vẽ hiệu ứng nổ
        if(cell == 'x' || cell == '@') {
            assets_render_image(renderer, &game->assets, "explosion.png",
                              x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        }

        // Nếu là ô miss (o) thì vẽ nước văng
        if(cell == 'o') {
            assets_render_image(renderer, &game->assets, "water_splash.png",
                              x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        }
    }
}
```

---

## 4. Thêm nhạc nền cho các màn hình

### 4.1. Nhạc nền khi vào lobby

Mở file: **`src/network/protocol.c`**

Tìm dòng parse `LOGIN_OK:` (dòng ~175):

```c
if(parse_login_response(game, msg)) {
    game->state = STATE_LOBBY;

    // THÊM: Phát nhạc nền lobby
    assets_play_music(&game->assets, "background.mp3", -1);  // -1 = lặp vô hạn

    return 1;
}
```

### 4.2. Dừng nhạc khi vào game

Tìm dòng parse `START_PLAYING` (dòng ~258):

```c
if(strncmp(msg, "START_PLAYING", 13) == 0) {
    game->state = STATE_PLAYING;
    game->is_my_turn = 0;

    // THÊM: Dừng nhạc nền lobby
    assets_stop_music();

    snprintf(game->message, sizeof(game->message), "Game started! Good luck!");
    return 1;
}
```

---

## 5. Âm thanh thắng/thua

Trong **`src/network/protocol.c`**:

### Khi thắng (dòng ~264):
```c
if(strncmp(msg, "YOU WIN:", 8) == 0) {
    game->state = STATE_GAME_OVER;

    // THÊM: Phát nhạc chiến thắng
    assets_play_sound(&game->assets, "victory.wav", 0);

    snprintf(game->message, sizeof(game->message), "YOU WIN!");
    return 1;
}
```

### Khi thua (dòng ~269):
```c
if(strncmp(msg, "YOU LOSE:", 9) == 0) {
    game->state = STATE_GAME_OVER;

    // THÊM: Phát nhạc thua cuộc
    assets_play_sound(&game->assets, "defeat.wav", 0);

    snprintf(game->message, sizeof(game->message), "YOU LOSE!");
    return 1;
}
```

---

## 6. Thêm ảnh nền cho màn hình

### Ví dụ: Ảnh nền cho login screen

Mở file: **`src/ui/screens/login_screen.c`**

Trong hàm `login_screen_render()`, ngay sau khi clear màn hình (dòng ~72):

```c
void login_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Tô nền màn hình màu xanh đen (20, 30, 50)
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);

    // THÊM: Vẽ ảnh nền (nếu có)
    assets_render_image(renderer, &game->assets, "background.jpg", 0, 0, 1000, 700);

    // ... (code còn lại)
}
```

---

## 7. Compile và chạy

Sau khi thêm code:

```bash
cd /home/chau/Battle_Ship/Battle_Ship/new_code/client

# Compile lại
make clean
make

# Chạy
./run_client.sh
```

---

## 8. Debug khi assets không hoạt động

### Kiểm tra terminal output:
Khi chạy game, terminal sẽ in:
```
DEBUG: Loading images...
Image 'explosion.png' loaded successfully! (64x64)
Image 'water_splash.png' loaded successfully! (64x64)
...
DEBUG: Loading sounds...
Sound 'shot.wav' loaded successfully!
...
```

Nếu thấy:
```
WARNING: Failed to load image 'xxx.png': Couldn't open xxx.png
```

→ File không tồn tại hoặc đường dẫn sai. Kiểm tra lại `assets/images/xxx.png`

### Kiểm tra file có đúng format không:
```bash
# Kiểm tra ảnh
file assets/images/explosion.png
# Output: PNG image data, 64 x 64, 8-bit/color RGBA

# Kiểm tra âm thanh
file assets/sounds/shot.wav
# Output: RIFF (little-endian) data, WAVE audio
```

---

## 9. Tips và best practices

### Performance:
- Load tất cả assets 1 lần khi khởi động
- **KHÔNG** load/play trong vòng lặp render
- Giới hạn kích thước: ảnh < 1MB, sound < 500KB

### Tổ chức code:
- Tất cả logic load assets trong `client_gui_complete.c`
- Chỉ gọi `assets_play_sound()` hoặc `assets_render_image()` trong các screen

### Kiểm tra assets có load không:
```c
// Kiểm tra trước khi dùng
SDL_Texture* tex = assets_get_image(&game->assets, "explosion.png");
if(tex) {
    // Có ảnh, an toàn để dùng
    assets_render_image(...);
}
```

---

## Checklist hoàn chỉnh

- [x] ✅ Hệ thống assets đã được tích hợp
- [ ] Copy file ảnh/âm thanh vào `assets/images/` và `assets/sounds/`
- [ ] Load assets trong `client_gui_complete.c` (dòng ~85)
- [ ] Thêm âm thanh bắn trong `playing_screen.c`
- [ ] Thêm âm thanh hit/miss trong `protocol.c`
- [ ] Thêm hiệu ứng ảnh trong `playing_screen_render()`
- [ ] Thêm nhạc nền trong các state transitions
- [ ] Test và kiểm tra terminal output

---

**Chúc may mắn!** Nếu cần hỗ trợ, hỏi Linh nhé! 🎮🎵

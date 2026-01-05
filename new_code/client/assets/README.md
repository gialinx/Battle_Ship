# Assets - Hướng dẫn thêm ảnh và âm thanh

Thư mục này chứa tất cả ảnh và âm thanh cho game.

## Cấu trúc thư mục

```
assets/
├── images/          → Chứa các file ảnh (.png, .jpg, .bmp)
│   ├── explosion.png
│   ├── ship.png
│   └── ...
│
└── sounds/          → Chứa các file âm thanh (.wav, .mp3, .ogg)
    ├── shot.wav
    ├── hit.wav
    ├── miss.wav
    ├── background.mp3
    └── ...
```

## Hướng dẫn thêm ảnh

### 1. Chuẩn bị file ảnh
- **Format hỗ trợ**: PNG, JPG, BMP, GIF
- **Khuyến nghị**: Dùng PNG cho ảnh có nền trong suốt
- **Kích thước**: Tùy ý (sẽ scale khi render)

### 2. Thêm ảnh vào project
```bash
# Copy file ảnh vào thư mục images/
cp /path/to/your/image.png assets/images/
```

### 3. Load ảnh trong code
Trong file `client_gui_complete.c`, sau khi khởi tạo SDL:

```c
// Load ảnh vào assets manager
assets_load_image(&game.assets, game.renderer, "explosion.png");
assets_load_image(&game.assets, game.renderer, "ship.png");
```

### 4. Vẽ ảnh lên màn hình
Trong bất kỳ file screen nào (VD: `playing_screen.c`):

```c
// Vẽ ảnh với kích thước gốc tại vị trí (x, y)
assets_render_image(renderer, &game->assets, "explosion.png", x, y, 0, 0);

// Vẽ ảnh với kích thước tùy chỉnh (width x height)
assets_render_image(renderer, &game->assets, "ship.png", x, y, 50, 50);
```

## Hướng dẫn thêm âm thanh

### 1. Chuẩn bị file âm thanh
- **Format hỗ trợ**: WAV, MP3, OGG
- **Khuyến nghị**:
  - Dùng WAV cho sound effects (nhỏ gọn, ít lag)
  - Dùng MP3/OGG cho nhạc nền (tiết kiệm dung lượng)

### 2. Thêm âm thanh vào project
```bash
# Copy file âm thanh vào thư mục sounds/
cp /path/to/your/sound.wav assets/sounds/
cp /path/to/your/music.mp3 assets/sounds/
```

### 3. Load âm thanh trong code
Trong file `client_gui_complete.c`:

```c
// Load sound effects
assets_load_sound(&game.assets, "shot.wav");
assets_load_sound(&game.assets, "hit.wav");
assets_load_sound(&game.assets, "miss.wav");

// Load nhạc nền
assets_load_music(&game.assets, "background.mp3");
```

### 4. Phát âm thanh
Trong bất kỳ screen nào:

```c
// Phát sound effect 1 lần
assets_play_sound(&game->assets, "shot.wav", 0);

// Phát sound effect lặp lại 3 lần
assets_play_sound(&game->assets, "explosion.wav", 2);  // 0 = 1 lần, 1 = 2 lần, ...

// Phát nhạc nền lặp vô hạn
assets_play_music(&game->assets, -1);  // -1 = lặp vô hạn

// Dừng nhạc nền
assets_stop_music();

// Dừng tất cả âm thanh
assets_stop_all_sounds();
```

## Ví dụ sử dụng trong game

### 1. Khi bắn vào ô địch (playing_screen.c)
```c
// Trong playing_screen_handle_click()
if(cell == '-') {
    // Bắn vào ô trống
    assets_play_sound(&game->assets, "shot.wav", 0);

    // Sau khi nhận kết quả từ server
    if(hit) {
        assets_play_sound(&game->assets, "hit.wav", 0);
        assets_render_image(renderer, &game->assets, "explosion.png", x, y, 40, 40);
    } else {
        assets_play_sound(&game->assets, "miss.wav", 0);
    }
}
```

### 2. Khi chuyển màn hình
```c
// Khi vào màn hình lobby
if(game->state == STATE_LOBBY) {
    assets_play_music(&game->assets, "lobby_music.mp3", -1);
}

// Khi vào màn hình chơi
if(game->state == STATE_PLAYING) {
    assets_stop_music();  // Dừng nhạc lobby
    assets_play_music(&game->assets, "battle_music.mp3", -1);
}
```

### 3. Khi thắng/thua
```c
// Trong protocol.c khi parse YOU WIN/YOU LOSE
if(strncmp(msg, "YOU WIN:", 8) == 0) {
    game->state = STATE_GAME_OVER;
    assets_play_sound(&game->assets, "victory.wav", 0);
    snprintf(game->message, sizeof(game->message), "YOU WIN!");
}

if(strncmp(msg, "YOU LOSE:", 9) == 0) {
    game->state = STATE_GAME_OVER;
    assets_play_sound(&game->assets, "defeat.wav", 0);
    snprintf(game->message, sizeof(game->message), "YOU LOSE!");
}
```

## Lưu ý quan trọng

### 1. Load assets khi khởi động
- Load tất cả ảnh/âm thanh 1 lần khi khởi động game
- **KHÔNG** load trong vòng lặp render (sẽ rất chậm!)

### 2. Giải phóng bộ nhớ
Assets sẽ tự động được giải phóng khi game tắt qua hàm `assets_cleanup()`

### 3. Giới hạn
- Tối đa **50 ảnh**
- Tối đa **50 âm thanh**
- **1 nhạc nền** tại 1 thời điểm

### 4. Performance
- Ảnh PNG nhỏ hơn 1MB: Tốt
- Sound effects nhỏ hơn 500KB: Tốt
- Nhạc nền: Không nên quá 5MB

## Checklist triển khai

- [ ] Chuẩn bị file ảnh/âm thanh
- [ ] Copy vào `assets/images/` hoặc `assets/sounds/`
- [ ] Load trong `client_gui_complete.c` (sau khi init SDL)
- [ ] Sử dụng trong các screen file
- [ ] Test xem ảnh/âm thanh hoạt động chưa

## Troubleshooting

**Q: Ảnh không hiển thị?**
- Kiểm tra file có trong `assets/images/` chưa
- Kiểm tra tên file đúng chưa (case-sensitive!)
- Xem terminal có warning "Failed to load image" không

**Q: Âm thanh không phát?**
- Kiểm tra file có trong `assets/sounds/` chưa
- Thử format WAV thay vì MP3
- Xem terminal có warning "Failed to load sound" không

**Q: Game bị lag khi phát âm thanh?**
- File âm thanh quá lớn, nên dùng file nhỏ hơn
- Convert về WAV với bitrate thấp hơn

## Ví dụ assets gợi ý

### Ảnh:
- `explosion.png` - Hiệu ứng nổ khi trúng tàu
- `ship_horizontal.png` - Hình tàu nằm ngang
- `ship_vertical.png` - Hình tàu thẳng đứng
- `water.png` - Background nước
- `logo.png` - Logo game

### Âm thanh:
- `shot.wav` - Tiếng bắn
- `hit.wav` - Tiếng trúng đích
- `miss.wav` - Tiếng trượt
- `sink.wav` - Tiếng tàu chìm
- `victory.wav` - Tiếng thắng
- `defeat.wav` - Tiếng thua
- `background.mp3` - Nhạc nền
- `button_click.wav` - Tiếng click button

---

**Có câu hỏi?** Hỏi Linh hoặc xem code trong `src/core/assets.c`

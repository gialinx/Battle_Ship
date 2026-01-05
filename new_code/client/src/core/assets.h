// assets.h - Quản lý ảnh và âm thanh trong game
#ifndef ASSETS_H
#define ASSETS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

// ==================== CẤU TRÚC DỮ LIỆU ====================

// Cấu trúc quản lý 1 texture (ảnh)
typedef struct {
    char name[50];              // Tên file (VD: "explosion.png")
    SDL_Texture* texture;       // Con trỏ tới texture đã load
    int width;                  // Chiều rộng ảnh
    int height;                 // Chiều cao ảnh
    int loaded;                 // 1 nếu đã load thành công, 0 nếu chưa
} ImageAsset;

// Cấu trúc quản lý 1 âm thanh
typedef struct {
    char name[50];              // Tên file (VD: "shot.wav")
    Mix_Chunk* chunk;           // Con trỏ tới âm thanh đã load
    int loaded;                 // 1 nếu đã load thành công, 0 nếu chưa
} SoundAsset;

// Cấu trúc quản lý tất cả assets
typedef struct {
    ImageAsset images[50];      // Mảng chứa tối đa 50 ảnh
    int image_count;            // Số lượng ảnh hiện tại

    SoundAsset sounds[50];      // Mảng chứa tối đa 50 âm thanh
    int sound_count;            // Số lượng âm thanh hiện tại

    Mix_Music* bg_music;        // Nhạc nền (nếu có)
    int music_loaded;           // 1 nếu đã load nhạc nền
} AssetsManager;

// ==================== KHỞI TẠO VÀ CLEANUP ====================

/**
 * Khởi tạo hệ thống assets (SDL_image và SDL_mixer)
 * @return 1 nếu thành công, 0 nếu thất bại
 */
int assets_init();

/**
 * Giải phóng tất cả assets và tắt hệ thống
 * @param assets - Con trỏ tới AssetsManager
 */
void assets_cleanup(AssetsManager* assets);

/**
 * Khởi tạo AssetsManager rỗng
 * @param assets - Con trỏ tới AssetsManager
 */
void assets_manager_init(AssetsManager* assets);

// ==================== LOAD ASSETS ====================

/**
 * Load một ảnh từ file
 * @param assets - Con trỏ tới AssetsManager
 * @param renderer - SDL Renderer để tạo texture
 * @param filename - Tên file ảnh (VD: "explosion.png")
 * @return 1 nếu thành công, 0 nếu thất bại
 *
 * Lưu ý: File phải nằm trong thư mục assets/images/
 */
int assets_load_image(AssetsManager* assets, SDL_Renderer* renderer, const char* filename);

/**
 * Load một âm thanh từ file
 * @param assets - Con trỏ tới AssetsManager
 * @param filename - Tên file âm thanh (VD: "shot.wav")
 * @return 1 nếu thành công, 0 nếu thất bại
 *
 * Lưu ý: File phải nằm trong thư mục assets/sounds/
 */
int assets_load_sound(AssetsManager* assets, const char* filename);

/**
 * Load nhạc nền từ file
 * @param assets - Con trỏ tới AssetsManager
 * @param filename - Tên file nhạc (VD: "background.mp3")
 * @return 1 nếu thành công, 0 nếu thất bại
 *
 * Lưu ý: File phải nằm trong thư mục assets/sounds/
 */
int assets_load_music(AssetsManager* assets, const char* filename);

// ==================== SỬ DỤNG ASSETS ====================

/**
 * Lấy texture của một ảnh theo tên
 * @param assets - Con trỏ tới AssetsManager
 * @param filename - Tên file ảnh
 * @return Con trỏ tới SDL_Texture, hoặc NULL nếu không tìm thấy
 */
SDL_Texture* assets_get_image(AssetsManager* assets, const char* filename);

/**
 * Vẽ một ảnh lên màn hình
 * @param renderer - SDL Renderer
 * @param assets - Con trỏ tới AssetsManager
 * @param filename - Tên file ảnh
 * @param x - Tọa độ x
 * @param y - Tọa độ y
 * @param width - Chiều rộng (0 = kích thước gốc)
 * @param height - Chiều cao (0 = kích thước gốc)
 */
void assets_render_image(SDL_Renderer* renderer, AssetsManager* assets,
                        const char* filename, int x, int y, int width, int height);

/**
 * Phát một âm thanh
 * @param assets - Con trỏ tới AssetsManager
 * @param filename - Tên file âm thanh
 * @param loops - Số lần lặp lại (0 = phát 1 lần, -1 = lặp vô hạn)
 */
void assets_play_sound(AssetsManager* assets, const char* filename, int loops);

/**
 * Phát nhạc nền
 * @param assets - Con trỏ tới AssetsManager
 * @param loops - Số lần lặp lại (-1 = lặp vô hạn)
 */
void assets_play_music(AssetsManager* assets, int loops);

/**
 * Dừng nhạc nền
 */
void assets_stop_music();

/**
 * Dừng tất cả âm thanh
 */
void assets_stop_all_sounds();

#endif // ASSETS_H

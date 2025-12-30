// assets.c - Triển khai quản lý ảnh và âm thanh
#include "assets.h"
#include <stdio.h>
#include <string.h>

// ==================== KHỞI TẠO ====================

int assets_init() {
    // Khởi tạo SDL_image (hỗ trợ PNG, JPG, ...)
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if(!(IMG_Init(img_flags) & img_flags)) {
        printf("ERROR: SDL_image init failed: %s\n", IMG_GetError());
        return 0;
    }

    // Khởi tạo SDL_mixer (hỗ trợ âm thanh) - OPTIONAL
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("WARNING: SDL_mixer init failed (audio disabled): %s\n", Mix_GetError());
        // Không return 0 - game vẫn chạy được không có âm thanh
    } else {
        printf("Audio system initialized\n");
    }

    printf("Assets system initialized successfully!\n");
    return 1;
}

void assets_manager_init(AssetsManager* assets) {
    // Reset tất cả về 0
    assets->image_count = 0;
    assets->sound_count = 0;
    assets->bg_music = NULL;
    assets->music_loaded = 0;

    // Đánh dấu tất cả assets là chưa load
    for(int i = 0; i < 50; i++) {
        assets->images[i].loaded = 0;
        assets->images[i].texture = NULL;
        assets->sounds[i].loaded = 0;
        assets->sounds[i].chunk = NULL;
    }
}

void assets_cleanup(AssetsManager* assets) {
    // Giải phóng tất cả ảnh
    for(int i = 0; i < assets->image_count; i++) {
        if(assets->images[i].loaded && assets->images[i].texture) {
            SDL_DestroyTexture(assets->images[i].texture);
            assets->images[i].texture = NULL;
        }
    }

    // Giải phóng tất cả âm thanh
    for(int i = 0; i < assets->sound_count; i++) {
        if(assets->sounds[i].loaded && assets->sounds[i].chunk) {
            Mix_FreeChunk(assets->sounds[i].chunk);
            assets->sounds[i].chunk = NULL;
        }
    }

    // Giải phóng nhạc nền
    if(assets->music_loaded && assets->bg_music) {
        Mix_FreeMusic(assets->bg_music);
        assets->bg_music = NULL;
    }

    // Tắt hệ thống
    Mix_CloseAudio();
    IMG_Quit();

    printf("Assets cleaned up!\n");
}

// ==================== LOAD ASSETS ====================

int assets_load_image(AssetsManager* assets, SDL_Renderer* renderer, const char* filename) {
    // Kiểm tra xem đã load chưa
    for(int i = 0; i < assets->image_count; i++) {
        if(strcmp(assets->images[i].name, filename) == 0) {
            printf("Image '%s' already loaded!\n", filename);
            return 1;
        }
    }

    // Kiểm tra giới hạn
    if(assets->image_count >= 50) {
        printf("ERROR: Too many images loaded (max 50)!\n");
        return 0;
    }

    // Tạo đường dẫn đầy đủ: assets/images/filename
    char path[256];
    snprintf(path, sizeof(path), "assets/images/%s", filename);

    // Load surface từ file
    SDL_Surface* surface = IMG_Load(path);
    if(!surface) {
        printf("WARNING: Failed to load image '%s': %s\n", path, IMG_GetError());
        return 0;
    }

    // Tạo texture từ surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    int width = surface->w;
    int height = surface->h;
    SDL_FreeSurface(surface);  // Giải phóng surface (không cần nữa)

    if(!texture) {
        printf("ERROR: Failed to create texture from '%s': %s\n", path, SDL_GetError());
        return 0;
    }

    // Lưu vào assets manager
    ImageAsset* img = &assets->images[assets->image_count];
    strncpy(img->name, filename, sizeof(img->name) - 1);
    img->name[sizeof(img->name) - 1] = '\0';
    img->texture = texture;
    img->width = width;
    img->height = height;
    img->loaded = 1;

    assets->image_count++;
    printf("Image '%s' loaded successfully! (%dx%d)\n", filename, width, height);
    return 1;
}

int assets_load_sound(AssetsManager* assets, const char* filename) {
    // Kiểm tra xem đã load chưa
    for(int i = 0; i < assets->sound_count; i++) {
        if(strcmp(assets->sounds[i].name, filename) == 0) {
            printf("Sound '%s' already loaded!\n", filename);
            return 1;
        }
    }

    // Kiểm tra giới hạn
    if(assets->sound_count >= 50) {
        printf("ERROR: Too many sounds loaded (max 50)!\n");
        return 0;
    }

    // Tạo đường dẫn đầy đủ: assets/sounds/filename
    char path[256];
    snprintf(path, sizeof(path), "assets/sounds/%s", filename);

    // Load âm thanh
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if(!chunk) {
        printf("WARNING: Failed to load sound '%s': %s\n", path, Mix_GetError());
        return 0;
    }

    // Lưu vào assets manager
    SoundAsset* snd = &assets->sounds[assets->sound_count];
    strncpy(snd->name, filename, sizeof(snd->name) - 1);
    snd->name[sizeof(snd->name) - 1] = '\0';
    snd->chunk = chunk;
    snd->loaded = 1;

    assets->sound_count++;
    printf("Sound '%s' loaded successfully!\n", filename);
    return 1;
}

int assets_load_music(AssetsManager* assets, const char* filename) {
    // Nếu đã có nhạc nền thì giải phóng
    if(assets->music_loaded && assets->bg_music) {
        Mix_FreeMusic(assets->bg_music);
        assets->bg_music = NULL;
    }

    // Tạo đường dẫn đầy đủ: assets/sounds/filename
    char path[256];
    snprintf(path, sizeof(path), "assets/sounds/%s", filename);

    // Load nhạc nền
    Mix_Music* music = Mix_LoadMUS(path);
    if(!music) {
        printf("WARNING: Failed to load music '%s': %s\n", path, Mix_GetError());
        return 0;
    }

    assets->bg_music = music;
    assets->music_loaded = 1;
    printf("Music '%s' loaded successfully!\n", filename);
    return 1;
}

// ==================== SỬ DỤNG ASSETS ====================

SDL_Texture* assets_get_image(AssetsManager* assets, const char* filename) {
    for(int i = 0; i < assets->image_count; i++) {
        if(strcmp(assets->images[i].name, filename) == 0) {
            return assets->images[i].texture;
        }
    }
    return NULL;  // Không tìm thấy
}

void assets_render_image(SDL_Renderer* renderer, AssetsManager* assets,
                        const char* filename, int x, int y, int width, int height) {
    // Tìm ảnh
    ImageAsset* img = NULL;
    for(int i = 0; i < assets->image_count; i++) {
        if(strcmp(assets->images[i].name, filename) == 0) {
            img = &assets->images[i];
            break;
        }
    }

    if(!img || !img->loaded) {
        // Không tìm thấy ảnh (không in warning để tránh spam)
        return;
    }

    // Tạo SDL_Rect để vẽ
    SDL_Rect dest_rect;
    dest_rect.x = x;
    dest_rect.y = y;
    dest_rect.w = (width > 0) ? width : img->width;   // Nếu width = 0 thì dùng kích thước gốc
    dest_rect.h = (height > 0) ? height : img->height;

    // Vẽ texture
    SDL_RenderCopy(renderer, img->texture, NULL, &dest_rect);
}

void assets_play_sound(AssetsManager* assets, const char* filename, int loops) {
    // Tìm âm thanh
    SoundAsset* snd = NULL;
    for(int i = 0; i < assets->sound_count; i++) {
        if(strcmp(assets->sounds[i].name, filename) == 0) {
            snd = &assets->sounds[i];
            break;
        }
    }

    if(!snd || !snd->loaded) {
        // Không tìm thấy âm thanh (không in warning)
        return;
    }

    // Phát âm thanh trên channel tự động (-1 = channel đầu tiên rảnh)
    Mix_PlayChannel(-1, snd->chunk, loops);
}

void assets_play_music(AssetsManager* assets, int loops) {
    if(!assets->music_loaded || !assets->bg_music) {
        return;
    }
    Mix_PlayMusic(assets->bg_music, loops);
}

void assets_stop_music() {
    Mix_HaltMusic();
}

void assets_stop_all_sounds() {
    Mix_HaltChannel(-1);  // -1 = tất cả channels
}

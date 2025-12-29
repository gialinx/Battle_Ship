// game_data.h - Game data structures và enums
#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "assets.h"  // Quản lý ảnh và âm thanh

// Constants
#define MAP_SIZE 13
#define BUFF_SIZE 8192

// ==================== GAME STATES ====================
typedef enum {
    STATE_LOGIN,           // Màn hình đăng nhập/đăng ký
    STATE_LOBBY,           // Màn hình lobby - chọn người chơi
    STATE_WAITING_INVITE,  // Đang chờ phản hồi lời mời
    STATE_RECEIVED_INVITE, // Nhận được lời mời
    STATE_PLACING_SHIPS,   // Đặt tàu
    STATE_WAITING_READY,   // Chờ đối thủ READY
    STATE_PLAYING,         // Đang chơi
    STATE_GAME_OVER        // Kết thúc
} GameState;

// ==================== USER INFO ====================
typedef struct {
    int user_id;
    char username[50];
    char status[20];  // "online" or "offline"
    int elo_rating;
} UserInfo;

// ==================== INPUT FIELD ====================
typedef struct {
    char text[50];
    int cursor_pos;
    int is_active;
    SDL_Rect rect;
    int is_password;
} InputField;

// ==================== GAME DATA STRUCTURE ====================
typedef struct {
    // SDL
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* font_small;

    // Assets (ảnh và âm thanh)
    AssetsManager assets;

    // Network
    int sockfd;
    pthread_t recv_thread;
    
    // Game state
    GameState state;
    int running;
    
    // Login/Register
    InputField username_field;
    InputField password_field;
    int is_register_mode;  // 0 = login, 1 = register
    char login_message[256];
    
    // User info
    int my_user_id;
    char my_username[50];
    int my_elo;
    int total_games;
    int wins;
    
    // Lobby
    UserInfo users[20];
    int user_count;
    int selected_user_index;
    int scroll_offset;
    
    // Invite system
    int invited_user_id;
    char invited_username[50];
    int inviter_user_id;
    char inviter_username[50];
    
    // Maps
    char own_map[MAP_SIZE][MAP_SIZE];
    char enemy_map[MAP_SIZE][MAP_SIZE];
    
    // Ship placement
    int selected_ship_length;
    int selected_ship_id;
    int ship_horizontal;
    int ships_to_place[4];             // {4, 3, 2, 2}
    int ships_placed_count[5];         // Index by ship length [0,1,2,3,4]
    int ships_placed_countMax[5];      // Max count for each ship length
    int mouse_grid_x;
    int mouse_grid_y;
    int preview_valid;
    int ships_placed;
    
    // Gameplay
    int is_my_turn;
    int game_active;
    char message[256];
    char game_message[256];
} GameData;

#endif



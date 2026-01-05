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
    STATE_PROFILE,         // Màn hình profile cá nhân
    STATE_MATCHMAKING,     // Đang tìm trận (matchmaking)
    STATE_MATCH_FOUND,     // Tìm được trận - chờ confirm
    STATE_SENDING_INVITE,  // Đang gửi lời mời (chờ server xác nhận)
    STATE_WAITING_INVITE,  // Đang chờ phản hồi lời mời (đã gửi thành công)
    STATE_RECEIVED_INVITE, // Nhận được lời mời
    STATE_PLACING_SHIPS,   // Đặt tàu
    STATE_WAITING_READY,   // Chờ đối thủ READY
    STATE_PLAYING,         // Đang chơi
    STATE_GAME_OVER        // Kết thúc
} GameState;

// ==================== LOBBY TABS ====================
typedef enum {
    LOBBY_TAB_LEADERBOARD,  // Tab bảng xếp hạng
    LOBBY_TAB_STATS,        // Tab thống kê cá nhân
    LOBBY_TAB_HISTORY       // Tab lịch sử trận đấu
} LobbyTab;

// ==================== USER INFO ====================
typedef struct {
    int user_id;
    char username[50];
    char status[20];  // "online" or "offline"
    int elo_rating;
    int in_game;      // 1 if currently playing, 0 if available
} UserInfo;

// ==================== INPUT FIELD ====================
typedef struct {
    char text[50];
    int cursor_pos;
    int is_active;
    SDL_Rect rect;
    int is_password;
} InputField;

// ==================== LEADERBOARD ENTRY ====================
typedef struct {
    int rank;
    char username[50];
    int elo_rating;
    int total_games;
    int wins;
} LeaderboardEntry;

// ==================== MATCH HISTORY ENTRY ====================
typedef struct {
    int match_id;
    char opponent_name[50];
    int my_elo_before;
    int my_elo_after;
    int elo_change;
    int result;  // 1 = win, 0 = lose
    char date[50];
} MatchHistoryEntry;

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
    unsigned int last_users_update;  // SDL_GetTicks() timestamp for auto-refresh

    // Lobby UI
    LobbyTab active_lobby_tab;
    InputField player_search_field;

    // Matchmaking
    int matchmaking_active;           // 1 nếu đang trong hàng chờ
    unsigned int matchmaking_start_time;  // SDL_GetTicks() khi bắt đầu matchmaking
    char matched_opponent_name[50];   // Tên đối thủ khi tìm được match
    int matched_opponent_id;          // ID đối thủ
    int matched_opponent_elo;         // ELO đối thủ

    // Leaderboard data
    LeaderboardEntry leaderboard[10];
    int leaderboard_count;
    unsigned int last_leaderboard_update;  // SDL_GetTicks() timestamp

    // Match history data
    MatchHistoryEntry match_history[10];
    int match_history_count;

    // Personal stats
    int losses;
    int rank;  // Hạng hiện tại trong leaderboard

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
    
    // Logout flag
    int logout_requested;

    // Gameplay
    int is_my_turn;
    int game_active;
    char message[256];
    char game_message[256];
    
    // Game result (for game over screen)
    int game_result_won;       // 1 = win, 0 = lose
    int elo_before;            // ELO trước trận
    int elo_change;            // ELO change (base, không bao gồm bonus)
    int elo_bonus;             // Performance bonus
    int elo_predicted;         // ELO dự đoán real-time (tăng khi bắn trúng)
    
    // Game statistics
    int total_shots;           // Tổng số shots đã bắn
    int hits_count;            // Số lần bắn trúng
    int misses_count;          // Số lần bắn trượt
} GameData;

#endif



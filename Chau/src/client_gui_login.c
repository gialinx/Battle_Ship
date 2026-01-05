// client_gui_login.c - Battleship với Login/Register UI và Lobby system
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <math.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700
#define MAP_SIZE 13
#define CELL_SIZE 30
#define MAX_USERS 20
#define BUFF_SIZE 8192

// ==================== COLORS ====================
#define COLOR_OCEAN (SDL_Color){100, 149, 237, 255}
#define COLOR_GRID (SDL_Color){70, 130, 180, 255}
#define COLOR_SHIP (SDL_Color){128, 128, 128, 255}
#define COLOR_SHIP_PREVIEW (SDL_Color){0, 255, 0, 200}
#define COLOR_SHIP_INVALID (SDL_Color){255, 0, 0, 200}
#define COLOR_HIT (SDL_Color){255, 69, 0, 255}
#define COLOR_MISS (SDL_Color){255, 255, 255, 255}
#define COLOR_SUNK (SDL_Color){50, 50, 50, 255}
#define COLOR_BG (SDL_Color){30, 60, 90, 255}
#define COLOR_TEXT (SDL_Color){255, 255, 255, 255}
#define COLOR_BUTTON (SDL_Color){70, 130, 180, 255}
#define COLOR_BUTTON_HOVER (SDL_Color){100, 160, 220, 255}
#define COLOR_BUTTON_DISABLED (SDL_Color){100, 100, 100, 255}


// ==================== GAME STATES ====================
typedef enum {
    STATE_LOGIN,           // Màn hình đăng nhập/đăng ký
    STATE_LOBBY,           // Màn hình lobby - chọn người chơi
    STATE_WAITING_INVITE,  // Đang chờ phản hồi lời mời
    STATE_RECEIVED_INVITE, // Nhận được lời mời
    STATE_PLACING_SHIPS,   // Đặt tàu
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

// ==================== GAME STRUCTURE ====================
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* font_small;
    
    GameState state;
    int sockfd;
    pthread_t recv_thread;
    
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
    UserInfo users[MAX_USERS];
    int user_count;
    int selected_user_index;
    int scroll_offset;
    
    // Invite system
    int invited_user_id;
    char invited_username[50];
    int inviter_user_id;
    char inviter_username[50];
    
    // Game data
    char own_map[MAP_SIZE][MAP_SIZE];
    char enemy_map[MAP_SIZE][MAP_SIZE];
    int ships_placed;
    int my_turn;
    int game_active;
    char game_message[256];
    
    // Ship placement data (from client_gui_v2.c)
    int selected_ship_length;
    int selected_ship_id;
    int ship_horizontal;
    int ships_to_place[4];             // {4, 3, 2, 2}
    int ships_placed_count[5];         // Index by ship length [0,1,2,3,4]
    int ships_placed_countMax[5];      // Max count for each ship length
    int mouse_grid_x;
    int mouse_grid_y;
    int preview_valid;
    int is_my_turn;
    char message[256];
    
    int running;
} Game;

Game game;
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;

// ==================== FORWARD DECLARATIONS ====================
void init_game();
void handle_events();
void render();
void send_to_server(const char* msg);
void* receive_thread(void* arg);

// ==================== INITIALIZE GAME ====================
void init_game() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    game.window = SDL_CreateWindow("Battleship - Login", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    
    game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED);
    game.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);
    game.font_small = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    
    game.state = STATE_LOGIN;
    game.running = 1;
    game.is_register_mode = 0;
    
    // Setup input fields
    game.username_field.rect = (SDL_Rect){300, 250, 400, 40};
    game.password_field.rect = (SDL_Rect){300, 320, 400, 40};
    game.username_field.is_password = 0;
    game.password_field.is_password = 1;
    game.username_field.is_active = 1;
    game.password_field.is_active = 0;
    
    strcpy(game.login_message, "");
    
    // Initialize maps
    for(int i=0; i<MAP_SIZE; i++)
        for(int j=0; j<MAP_SIZE; j++) {
            game.own_map[i][j] = '-';
            game.enemy_map[i][j] = '-';
        }
    
    // Initialize ship placement data
    game.selected_ship_length = 0;
    game.selected_ship_id = -1;
    game.ship_horizontal = 1;
    game.ships_to_place[0] = 4;
    game.ships_to_place[1] = 3;
    game.ships_to_place[2] = 2;
    game.ships_to_place[3] = 2;
    for(int i = 0; i < 5; i++) {
        game.ships_placed_count[i] = 0;
    }
    game.ships_placed_countMax[2] = 2;
    game.ships_placed_countMax[3] = 1;
    game.ships_placed_countMax[4] = 1;
    game.mouse_grid_x = -1;
    game.mouse_grid_y = -1;
    game.preview_valid = 0;
    game.is_my_turn = 0;
    strcpy(game.message, "");
    
    // Connect to server
    game.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5500);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    
    if(connect(game.sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Connection failed!\n");
        exit(1);
    }
    
    pthread_create(&game.recv_thread, NULL, receive_thread, NULL);
    printf("Connected to server\n");
}

// ==================== SEND TO SERVER ====================
void send_to_server(const char* msg) {
    send(game.sockfd, msg, strlen(msg), 0);
    printf("SENT: %s\n", msg);
}

// ==================== RENDER TEXT ====================
void render_text(const char* text, int x, int y, SDL_Color color, TTF_Font* font) {
    if(!font) font = game.font;
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(game.renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(game.renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// ==================== RENDER INPUT FIELD ====================
void render_input_field(InputField* field, const char* label) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {200, 200, 200, 255};
    SDL_Color black = {0, 0, 0, 255};
    
    // Label
    render_text(label, field->rect.x, field->rect.y - 30, white, game.font_small);
    
    // Box
    SDL_SetRenderDrawColor(game.renderer, 
        field->is_active ? 100 : 60, 
        field->is_active ? 100 : 60, 
        field->is_active ? 100 : 60, 255);
    SDL_RenderFillRect(game.renderer, &field->rect);
    
    SDL_SetRenderDrawColor(game.renderer, 
        field->is_active ? 255 : 150, 
        field->is_active ? 255 : 150, 
        field->is_active ? 255 : 150, 255);
    SDL_RenderDrawRect(game.renderer, &field->rect);
    
    // Text
    if(strlen(field->text) > 0) {
        char display[50];
        if(field->is_password) {
            for(int i=0; i<strlen(field->text); i++)
                display[i] = '*';
            display[strlen(field->text)] = '\0';
        } else {
            strcpy(display, field->text);
        }
        render_text(display, field->rect.x + 10, field->rect.y + 8, white, game.font_small);
    }
}

// ==================== RENDER BUTTON ====================
int render_button(const char* text, int x, int y, int w, int h, SDL_Color bg_color) {
    SDL_Rect rect = {x, y, w, h};
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int hover = (mx >= x && mx <= x+w && my >= y && my <= y+h);
    
    if(hover) {
        SDL_SetRenderDrawColor(game.renderer, 
            bg_color.r + 30, bg_color.g + 30, bg_color.b + 30, 255);
    } else {
        SDL_SetRenderDrawColor(game.renderer, bg_color.r, bg_color.g, bg_color.b, 255);
    }
    SDL_RenderFillRect(game.renderer, &rect);
    
    SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(game.renderer, &rect);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(game.font_small, text, white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(game.renderer, surface);
    SDL_Rect text_rect = {x + (w - surface->w)/2, y + (h - surface->h)/2, surface->w, surface->h};
    SDL_RenderCopy(game.renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    
    return hover;
}

// ==================== GAME HELPER FUNCTIONS ====================
void draw_text(const char *text, int x, int y, SDL_Color color, TTF_Font *font) {
    SDL_Surface *surface = TTF_RenderUTF8_Solid(font, text, color);
    if(!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(game.renderer, surface);
    if(!texture) { SDL_FreeSurface(surface); return; }
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(game.renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

int draw_button(const char *text, int x, int y, int w, int h, int enabled, int hover) {
    SDL_Rect rect = {x, y, w, h};
    if(!enabled) {
        SDL_SetRenderDrawColor(game.renderer, COLOR_BUTTON_DISABLED.r, COLOR_BUTTON_DISABLED.g, COLOR_BUTTON_DISABLED.b, 255);
    } else if(hover) {
        SDL_SetRenderDrawColor(game.renderer, COLOR_BUTTON_HOVER.r, COLOR_BUTTON_HOVER.g, COLOR_BUTTON_HOVER.b, 255);
    } else {
        SDL_SetRenderDrawColor(game.renderer, COLOR_BUTTON.r, COLOR_BUTTON.g, COLOR_BUTTON.b, 255);
    }
    SDL_RenderFillRect(game.renderer, &rect);
    SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(game.renderer, &rect);
    
    int text_len = strlen(text);
    draw_text(text, x + w/2 - text_len*4, y + h/2 - 10, COLOR_TEXT, game.font);
    return 1;
}

void draw_map(char map[MAP_SIZE][MAP_SIZE], int start_x, int start_y, int is_own_map) {
    for(int i = 0; i < MAP_SIZE; i++) {
        char num[3]; snprintf(num, 3, "%d", i + 1);
        draw_text(num, start_x + i * CELL_SIZE + CELL_SIZE/2 - 5, start_y - 25, COLOR_TEXT, game.font_small);
        draw_text(num, start_x - 30, start_y + i * CELL_SIZE + CELL_SIZE/2 - 8, COLOR_TEXT, game.font_small);
    }
    
    for(int row = 0; row < MAP_SIZE; row++) {
        for(int col = 0; col < MAP_SIZE; col++) {
            SDL_Rect rect = {start_x + col * CELL_SIZE, start_y + row * CELL_SIZE, CELL_SIZE - 2, CELL_SIZE - 2};
            char cell = map[row][col];
            
            if(cell == '-') {
                SDL_SetRenderDrawColor(game.renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            } else if(cell >= '2' && cell <= '9') {
                if(is_own_map) SDL_SetRenderDrawColor(game.renderer, COLOR_SHIP.r, COLOR_SHIP.g, COLOR_SHIP.b, 255);
                else SDL_SetRenderDrawColor(game.renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            } else if(cell == 'x') {
                SDL_SetRenderDrawColor(game.renderer, COLOR_MISS.r, COLOR_MISS.g, COLOR_MISS.b, 255);
            } else if(cell == 'o') {
                SDL_SetRenderDrawColor(game.renderer, COLOR_HIT.r, COLOR_HIT.g, COLOR_HIT.b, 255);
            } else if(cell == '@') {
                SDL_SetRenderDrawColor(game.renderer, COLOR_SUNK.r, COLOR_SUNK.g, COLOR_SUNK.b, 255);
            } else {
                SDL_SetRenderDrawColor(game.renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            }
            
            SDL_RenderFillRect(game.renderer, &rect);
            SDL_SetRenderDrawColor(game.renderer, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, 255);
            SDL_RenderDrawRect(game.renderer, &rect);
            
            if(cell == 'x') {
                SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(game.renderer, rect.x + 5, rect.y + 5, rect.x + rect.w - 5, rect.y + rect.h - 5);
                SDL_RenderDrawLine(game.renderer, rect.x + rect.w - 5, rect.y + 5, rect.x + 5, rect.y + rect.h - 5);
            } else if(cell == 'o' || cell == '@') {
                SDL_SetRenderDrawColor(game.renderer, 255, 255, 0, 255);
                int cx = rect.x + rect.w/2, cy = rect.y + rect.h/2;
                for(int r = 8; r <= 12; r++) {
                    for(int angle = 0; angle < 360; angle += 10) {
                        SDL_RenderDrawPoint(game.renderer, cx + r * SDL_cos(angle * M_PI / 180), cy + r * SDL_sin(angle * M_PI / 180));
                    }
                }
            }
        }
    }
}

int check_placement_valid(int x, int y, int length, int horizontal) {
    if(x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE) return 0;
    
    int dx = horizontal ? 1 : 0;
    int dy = horizontal ? 0 : 1;
    int ex = x + dx * (length - 1);
    int ey = y + dy * (length - 1);
    
    if(ex >= MAP_SIZE || ey >= MAP_SIZE) return 0;
    
    for(int i = 0; i < length; i++) {
        int tx = x + dx * i;
        int ty = y + dy * i;
        if(game.own_map[ty][tx] != '-') return 0;
    }
    return 1;
}

void draw_ship_preview(int start_x, int start_y) {
    if(game.selected_ship_length == 0) return;
    if(game.selected_ship_length == -1) return;
    if(game.mouse_grid_x < 0 || game.mouse_grid_y < 0) return;
    
    int length = game.selected_ship_length;
    int x = game.mouse_grid_x;
    int y = game.mouse_grid_y;
    int horizontal = game.ship_horizontal;
    
    int valid = check_placement_valid(x, y, length, horizontal);
    game.preview_valid = valid;
    
    SDL_Color color = valid ? COLOR_SHIP_PREVIEW : COLOR_SHIP_INVALID;
    SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);
    
    for(int i = 0; i < length; i++) {
        int tx = x + (horizontal ? i : 0);
        int ty = y + (horizontal ? 0 : i);
        if(tx >= 0 && tx < MAP_SIZE && ty >= 0 && ty < MAP_SIZE) {
            SDL_Rect rect = {start_x + tx * CELL_SIZE, start_y + ty * CELL_SIZE, CELL_SIZE - 2, CELL_SIZE - 2};
            SDL_SetRenderDrawColor(game.renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(game.renderer, &rect);
        }
    }
    SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_NONE);
}

// ==================== RENDER LOGIN SCREEN ====================
void render_login_screen() {
    SDL_SetRenderDrawColor(game.renderer, 20, 30, 50, 255);
    SDL_RenderClear(game.renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color red = {255, 50, 50, 255};
    
    // Title
    render_text(game.is_register_mode ? "SIGN UP" : "LOG IN", 
                350, 100, cyan, game.font);
    
    // Input fields
    render_input_field(&game.username_field, "Username:");
    render_input_field(&game.password_field, "Password:");
    
    // Buttons
    SDL_Color green = {0, 150, 0, 255};
    SDL_Color blue = {0, 100, 200, 255};
    
    if(render_button(game.is_register_mode ? "SIGN UP" : "LOG IN", 
                     350, 400, 150, 50, green)) {
        // Handle in mouse click
    }
    
    if(render_button(game.is_register_mode ? "Already have an account?" : "Create an account", 
                     520, 400, 180, 50, blue)) {
        // Handle in mouse click
    }
    
    // Message
    if(strlen(game.login_message) > 0) {
        render_text(game.login_message, 300, 480, 
                    strstr(game.login_message, "OK") ? cyan : red, 
                    game.font_small);
    }
}

// ==================== RENDER LOBBY SCREEN ====================
void render_lobby_screen() {
    SDL_SetRenderDrawColor(game.renderer, 20, 30, 50, 255);
    SDL_RenderClear(game.renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color gray = {150, 150, 150, 255};
    
    // Header
    char header[256];
    snprintf(header, sizeof(header), "Welcome %s! (ELO: %d, Games: %d, Wins: %d)", 
             game.my_username, game.my_elo, game.total_games, game.wins);
    render_text(header, 50, 20, cyan, game.font_small);
    
    render_text("ONLINE PLAYERS LIST", 350, 60, white, game.font);
    
    // User list
    int y = 120;
    for(int i = game.scroll_offset; i < game.user_count && i < game.scroll_offset + 10; i++) {
        UserInfo* u = &game.users[i];
        
        if(u->user_id == game.my_user_id) continue; // Skip self
        
        SDL_Rect user_rect = {50, y, 900, 45};
        
        // Background
        if(i == game.selected_user_index) {
            SDL_SetRenderDrawColor(game.renderer, 0, 80, 120, 255);
        } else {
            SDL_SetRenderDrawColor(game.renderer, 40, 50, 70, 255);
        }
        SDL_RenderFillRect(game.renderer, &user_rect);
        
        SDL_SetRenderDrawColor(game.renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(game.renderer, &user_rect);
        
        // Username
        char user_text[256];
        snprintf(user_text, sizeof(user_text), "%s (ELO: %d)", u->username, u->elo_rating);
        render_text(user_text, 60, y + 10, white, game.font_small);
        
        // Status
        SDL_Color status_color = strcmp(u->status, "online") == 0 ? green : gray;
        render_text(u->status, 700, y + 10, status_color, game.font_small);
        
        // Invite button (only for online users)
        if(strcmp(u->status, "online") == 0) {
            render_button("Invite to play", 800, y + 5, 120, 35, (SDL_Color){0, 150, 0, 255});
        }
        
        y += 50;
    }
    
    // Refresh button
    render_button("Refresh", 50, 620, 150, 50, (SDL_Color){0, 100, 200, 255});
    
    // Logout button
    render_button("Logout", 220, 620, 150, 50, (SDL_Color){200, 0, 0, 255});
}

// ==================== RENDER INVITE DIALOG ====================
void render_invite_dialog() {
    // Darken background
    SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(game.renderer, &overlay);
    
    // Dialog box
    SDL_Rect dialog = {250, 200, 500, 300};
    SDL_SetRenderDrawColor(game.renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(game.renderer, &dialog);
    SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(game.renderer, &dialog);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    
    if(game.state == STATE_WAITING_INVITE) {
        render_text("Waiting for response...", 320, 250, cyan, game.font);
        char msg[256];
        snprintf(msg, sizeof(msg), "Invitation sent to %s", game.invited_username);
        render_text(msg, 300, 300, white, game.font_small);

        render_button("Cancel", 400, 400, 200, 50, (SDL_Color){200, 0, 0, 255});
    } 
    else if(game.state == STATE_RECEIVED_INVITE) {
        render_text("Game invitation!", 340, 250, cyan, game.font);
        char msg[256];
        snprintf(msg, sizeof(msg), "%s wants to play a game with you", game.inviter_username);
        render_text(msg, 270, 300, white, game.font_small);

        render_button("Accept", 280, 400, 180, 50, (SDL_Color){0, 150, 0, 255});
        render_button("Decline", 480, 400, 180, 50, (SDL_Color){200, 0, 0, 255});
    }
}

// ==================== RENDER PLACING SHIPS ====================
void render_placing_ships() {
    SDL_SetRenderDrawColor(game.renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(game.renderer);
    
    // Title - use normal font
    draw_text("DAT TAU - BATTLESHIP", 320, 10, COLOR_TEXT, game.font_small);
    draw_text("Nhan [R] de xoay tau, Click chuot de dat", 250, 35, (SDL_Color){180,180,180,255}, game.font_small);
    
    // Danh sách tàu - moved left panel
    int list_x = 20, list_y = 70;
    draw_text("CHON TAU:", list_x, list_y, COLOR_TEXT, game.font_small);
    
    const char *ship_names[] = {"Lon (4o)", "Vua (3o)", "Nho (2o)"};
    int ship_lengths[] = {4, 3, 2};
    
    for(int i = 0; i < 3; i++) {
        int y = list_y + 30 + i * 55;
        int max_count = (ship_lengths[i] == 2) ? 2 : 1;
        int placed_count = game.ships_placed_count[ship_lengths[i]];
        int placed = (placed_count >= max_count);
        
        char btn_text[64];
        snprintf(btn_text, 64, "%s [%d/%d]", ship_names[i], placed_count, max_count);
        
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        int hover = (mx >= list_x && mx <= list_x + 150 && my >= y && my <= y + 40);
        
        draw_button(btn_text, list_x, y, 150, 40, !placed, hover);
        
        // Yellow border for selected ship
        if(game.selected_ship_length == ship_lengths[i] && !placed && game.selected_ship_id == i) {
            SDL_Rect rect = {list_x - 3, y - 3, 156, 46};
            SDL_SetRenderDrawColor(game.renderer, 255, 255, 0, 255);
            for(int w = 0; w < 3; w++) {
                SDL_Rect border = {rect.x - w, rect.y - w, rect.w + w*2, rect.h + w*2};
                SDL_RenderDrawRect(game.renderer, &border);
            }
        }
        
        // Green checkmark for completed
        if(placed) {
            draw_text("✓", list_x + 120, y + 10, (SDL_Color){0,255,0,255}, game.font_small);
        }
    }
    
    // Instructions box
    int info_y = list_y + 200;
    draw_text("HUONG DAN:", list_x, info_y, (SDL_Color){255,200,100,255}, game.font_small);
    draw_text("1. Chon tau", list_x, info_y + 25, (SDL_Color){200,200,200,255}, game.font_small);
    draw_text("2. Nhan R xoay", list_x, info_y + 45, (SDL_Color){200,200,200,255}, game.font_small);
    draw_text("3. Click dat", list_x, info_y + 65, (SDL_Color){200,200,200,255}, game.font_small);
    draw_text("4. READY choi", list_x, info_y + 85, (SDL_Color){200,200,200,255}, game.font_small);
    
    // Vẽ bản đồ - adjusted position and size
    int map_x = 200, map_y = 100;
    int cell_display = 28; // Smaller cells to fit
    draw_text("BAN DO CUA BAN", map_x + 140, map_y - 30, COLOR_TEXT, game.font_small);
    
    // Draw custom smaller map
    for(int row = 0; row < MAP_SIZE; row++) {
        for(int col = 0; col < MAP_SIZE; col++) {
            SDL_Rect rect = {map_x + col * cell_display, map_y + row * cell_display, cell_display - 2, cell_display - 2};
            char cell = game.own_map[row][col];
            
            if(cell == '-') {
                SDL_SetRenderDrawColor(game.renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            } else if(cell >= '2' && cell <= '9') {
                SDL_SetRenderDrawColor(game.renderer, COLOR_SHIP.r, COLOR_SHIP.g, COLOR_SHIP.b, 255);
            } else {
                SDL_SetRenderDrawColor(game.renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            }
            
            SDL_RenderFillRect(game.renderer, &rect);
            SDL_SetRenderDrawColor(game.renderer, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, 255);
            SDL_RenderDrawRect(game.renderer, &rect);
        }
    }
    
    // Draw ship preview with smaller cells
    if(game.selected_ship_length > 0 && game.mouse_grid_x >= 0 && game.mouse_grid_y >= 0) {
        int length = game.selected_ship_length;
        int x = game.mouse_grid_x;
        int y = game.mouse_grid_y;
        int horizontal = game.ship_horizontal;
        
        int valid = check_placement_valid(x, y, length, horizontal);
        game.preview_valid = valid;
        
        SDL_Color color = valid ? COLOR_SHIP_PREVIEW : COLOR_SHIP_INVALID;
        SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);
        
        for(int i = 0; i < length; i++) {
            int tx = x + (horizontal ? i : 0);
            int ty = y + (horizontal ? 0 : i);
            if(tx >= 0 && tx < MAP_SIZE && ty >= 0 && ty < MAP_SIZE) {
                SDL_Rect rect = {map_x + tx * cell_display, map_y + ty * cell_display, cell_display - 2, cell_display - 2};
                SDL_SetRenderDrawColor(game.renderer, color.r, color.g, color.b, color.a);
                SDL_RenderFillRect(game.renderer, &rect);
            }
        }
        SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_NONE);
    }
    
    // Button READY - repositioned
    int all_placed = (game.ships_placed_count[4] >= 1 && game.ships_placed_count[3] >= 1 && game.ships_placed_count[2] >= 2);
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int ready_x = map_x + 150, ready_y = map_y + MAP_SIZE * cell_display + 20;
    int ready_hover = (mx >= ready_x && mx <= ready_x + 150 && my >= ready_y && my <= ready_y + 50);
    
    if(all_placed) {
        draw_button("READY!", ready_x, ready_y, 150, 50, 1, ready_hover);
    } else {
        draw_button("Can dat 4 tau", ready_x, ready_y, 150, 50, 0, 0);
    }
    
    // Message at bottom
    if(strlen(game.message) > 0) {
        draw_text(game.message, 200, SCREEN_HEIGHT - 40, (SDL_Color){255,255,100,255}, game.font_small);
    }
    
    // Status counter
    char status[128];
    snprintf(status, sizeof(status), "Da dat: %d/4 tau", 
             game.ships_placed_count[4] + game.ships_placed_count[3] + game.ships_placed_count[2]);
    draw_text(status, 200, SCREEN_HEIGHT - 60, (SDL_Color){100,200,255,255}, game.font_small);
}

// ==================== RENDER PLAYING ====================
void render_playing() {
    SDL_SetRenderDrawColor(game.renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(game.renderer);
    
    draw_text("BATTLESHIP - TRAN DAU", 350, 10, COLOR_TEXT, game.font);
    
    if(game.is_my_turn) {
        draw_text("LUOT CUA BAN!", 400, 40, (SDL_Color){0, 255, 0, 255}, game.font);
    } else {
        draw_text("Cho doi thu...", 400, 40, (SDL_Color){255, 255, 0, 255}, game.font);
    }
    
    draw_text("BAN DO CUA BAN", 80, 70, COLOR_TEXT, game.font);
    draw_text("BAN DO DOI THU", 580, 70, COLOR_TEXT, game.font);
    
    draw_map(game.own_map, 30, 100, 1);
    draw_map(game.enemy_map, 530, 100, 0);
    
    if(strlen(game.message) > 0) {
        draw_text(game.message, 30, 650, (SDL_Color){255, 255, 0, 255}, game.font);
    }
}


// ==================== HANDLE LOGIN CLICK ====================
void handle_login_click(int x, int y) {
    // Check input field clicks
    if(x >= game.username_field.rect.x && x <= game.username_field.rect.x + game.username_field.rect.w &&
       y >= game.username_field.rect.y && y <= game.username_field.rect.y + game.username_field.rect.h) {
        game.username_field.is_active = 1;
        game.password_field.is_active = 0;
        return;
    }
    
    if(x >= game.password_field.rect.x && x <= game.password_field.rect.x + game.password_field.rect.w &&
       y >= game.password_field.rect.y && y <= game.password_field.rect.y + game.password_field.rect.h) {
        game.username_field.is_active = 0;
        game.password_field.is_active = 1;
        return;
    }
    
    // Login/Register button
    if(x >= 350 && x <= 500 && y >= 400 && y <= 450) {
        if(strlen(game.username_field.text) == 0 || strlen(game.password_field.text) == 0) {
            strcpy(game.login_message, "Please enter all information!");
            return;
        }
        
        char msg[256];
        if(game.is_register_mode) {
            snprintf(msg, sizeof(msg), "REGISTER:%s:%s#", 
                     game.username_field.text, game.password_field.text);
        } else {
            snprintf(msg, sizeof(msg), "LOGIN:%s:%s#", 
                     game.username_field.text, game.password_field.text);
        }
        send_to_server(msg);
        strcpy(game.login_message, "Processing...");
    }
    
    // Toggle mode button
    if(x >= 520 && x <= 700 && y >= 400 && y <= 450) {
        game.is_register_mode = !game.is_register_mode;
        strcpy(game.login_message, "");
    }
}

// ==================== HANDLE LOBBY CLICK ====================
void handle_lobby_click(int x, int y) {
    // Refresh button
    if(x >= 50 && x <= 200 && y >= 620 && y <= 670) {
        send_to_server("GET_USERS#");
        return;
    }
    
    // Logout button
    if(x >= 220 && x <= 370 && y >= 620 && y <= 670) {
        send_to_server("LOGOUT#");
        game.state = STATE_LOGIN;
        memset(&game.username_field.text, 0, sizeof(game.username_field.text));
        memset(&game.password_field.text, 0, sizeof(game.password_field.text));
        strcpy(game.login_message, "Logged out");
        return;
    }
    
    // User list clicks
    int list_y = 120;
    for(int i = game.scroll_offset; i < game.user_count && i < game.scroll_offset + 10; i++) {
        UserInfo* u = &game.users[i];
        
        if(u->user_id == game.my_user_id) continue;
        
        // Select user
        if(x >= 50 && x <= 950 && y >= list_y && y <= list_y + 45) {
            game.selected_user_index = i;
            
            // Invite button
            if(x >= 800 && x <= 920 && strcmp(u->status, "online") == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg), "INVITE:%d#", u->user_id);
                send_to_server(msg);
                
                game.invited_user_id = u->user_id;
                strcpy(game.invited_username, u->username);
                game.state = STATE_WAITING_INVITE;
            }
            return;
        }
        list_y += 50;
    }
}

// ==================== HANDLE INVITE DIALOG CLICK ====================
void handle_invite_dialog_click(int x, int y) {
    if(game.state == STATE_WAITING_INVITE) {
        // Cancel button
        if(x >= 400 && x <= 600 && y >= 400 && y <= 450) {
            send_to_server("CANCEL_INVITE#");
            game.state = STATE_LOBBY;
        }
    } 
    else if(game.state == STATE_RECEIVED_INVITE) {
        // Accept button
        if(x >= 280 && x <= 460 && y >= 400 && y <= 450) {
            char msg[256];
            snprintf(msg, sizeof(msg), "ACCEPT_INVITE:%d#", game.inviter_user_id);
            send_to_server(msg);
        }
        // Decline button
        else if(x >= 480 && x <= 660 && y >= 400 && y <= 450) {
            char msg[256];
            snprintf(msg, sizeof(msg), "DECLINE_INVITE:%d#", game.inviter_user_id);
            send_to_server(msg);
            game.state = STATE_LOBBY;
        }
    }
}

// ==================== HANDLE PLACING SHIPS CLICK ====================
void handle_placing_ships_click(int x, int y) {
    // Chọn tàu từ danh sách - updated coordinates
    int list_x = 20, list_y = 100;
    int ship_lengths[] = {4, 3, 2};
    
    for(int i = 0; i < 3; i++) {
        int btn_y = list_y + i * 55;
        if(x >= list_x && x <= list_x + 150 && y >= btn_y && y <= btn_y + 40) {
            int max_count = (ship_lengths[i] == 2) ? 2 : 1;
            int placed_count = game.ships_placed_count[ship_lengths[i]];
            if(placed_count < max_count) {
                game.selected_ship_length = ship_lengths[i];
                game.selected_ship_id = i;
                snprintf(game.message, sizeof(game.message), "Da chon tau %d o", ship_lengths[i]);
            }
            return;
        }
    }
    
    // Click vào bản đồ để đặt tàu - updated with smaller cells
    int map_x = 200, map_y = 100;
    int cell_display = 28;
    if(x >= map_x && x < map_x + MAP_SIZE * cell_display &&
       y >= map_y && y < map_y + MAP_SIZE * cell_display) {
        int grid_x = (x - map_x) / cell_display;
        int grid_y = (y - map_y) / cell_display;
        if(game.selected_ship_length > 0 && game.preview_valid) {
            // Place ship
            int length = game.selected_ship_length;
            int horizontal = game.ship_horizontal;
            
            // Mark on map
            for(int i = 0; i < length; i++) {
                int tx = grid_x + (horizontal ? i : 0);
                int ty = grid_y + (horizontal ? 0 : i);
                game.own_map[ty][tx] = '0' + length; // '2', '3', or '4'
            }
            
            game.ships_placed_count[length]++;
            snprintf(game.message, sizeof(game.message), "Da dat tau %d o! (%d/%d)", 
                     length, game.ships_placed_count[length], (length == 2 ? 2 : 1));
            
            game.selected_ship_length = 0;
            game.selected_ship_id = -1;
        }
    }
    
    // Click READY button - updated position
    int ready_x = map_x + 150, ready_y = map_y + MAP_SIZE * cell_display + 20;
    if(x >= ready_x && x <= ready_x + 150 && y >= ready_y && y <= ready_y + 50) {
        int all_placed = (game.ships_placed_count[4] >= 1 && 
                         game.ships_placed_count[3] >= 1 && 
                         game.ships_placed_count[2] >= 2);
        if(all_placed) {
            send_to_server("READY#");
            snprintf(game.message, sizeof(game.message), "Cho doi thu READY...");
        }
    }
}

// ==================== HANDLE PLAYING CLICK ====================
void handle_playing_click(int x, int y) {
    if(!game.is_my_turn) return;
    
    int start_x = 530, start_y = 100;
    if(x < start_x || x > start_x + MAP_SIZE * CELL_SIZE) return;
    if(y < start_y || y > start_y + MAP_SIZE * CELL_SIZE) return;
    
    int col = (x - start_x) / CELL_SIZE;
    int row = (y - start_y) / CELL_SIZE;
    if(col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return;
    
    char cell = game.enemy_map[row][col];
    if(cell == 'x' || cell == 'o' || cell == '@') {
        snprintf(game.message, sizeof(game.message), "Da ban o nay roi!");
        return;
    }
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "FIRE:%d,%d#", col + 1, row + 1);
    send_to_server(cmd);
    snprintf(game.message, sizeof(game.message), "Ban vao (%d,%d)...", col + 1, row + 1);
}

// ==================== HANDLE EVENTS ====================
void handle_events() {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) {
            game.running = 0;
        }
        
        // Mouse motion for ship preview
        if(e.type == SDL_MOUSEMOTION && game.state == STATE_PLACING_SHIPS) {
            int map_x = 200, map_y = 100;
            int cell_display = 28;
            if(e.motion.x >= map_x && e.motion.x < map_x + MAP_SIZE * cell_display &&
               e.motion.y >= map_y && e.motion.y < map_y + MAP_SIZE * cell_display) {
                game.mouse_grid_x = (e.motion.x - map_x) / cell_display;
                game.mouse_grid_y = (e.motion.y - map_y) / cell_display;
            } else {
                game.mouse_grid_x = -1;
                game.mouse_grid_y = -1;
            }
        }
        
        if(e.type == SDL_MOUSEBUTTONDOWN) {
            int x = e.button.x;
            int y = e.button.y;
            
            if(game.state == STATE_LOGIN) {
                handle_login_click(x, y);
            } else if(game.state == STATE_LOBBY) {
                handle_lobby_click(x, y);
            } else if(game.state == STATE_WAITING_INVITE || game.state == STATE_RECEIVED_INVITE) {
                handle_invite_dialog_click(x, y);
            } else if(game.state == STATE_PLACING_SHIPS) {
                handle_placing_ships_click(x, y);
            } else if(game.state == STATE_PLAYING) {
                handle_playing_click(x, y);
            }
        }
        
        if(e.type == SDL_TEXTINPUT && game.state == STATE_LOGIN) {
            InputField* active = game.username_field.is_active ? &game.username_field : &game.password_field;
            if(strlen(active->text) < 49) {
                strcat(active->text, e.text.text);
            }
        }
        
        if(e.type == SDL_KEYDOWN) {
            if(game.state == STATE_LOGIN) {
                InputField* active = game.username_field.is_active ? &game.username_field : &game.password_field;
                
                if(e.key.keysym.sym == SDLK_BACKSPACE && strlen(active->text) > 0) {
                    active->text[strlen(active->text) - 1] = '\0';
                } else if(e.key.keysym.sym == SDLK_TAB) {
                    game.username_field.is_active = !game.username_field.is_active;
                    game.password_field.is_active = !game.password_field.is_active;
                } else if(e.key.keysym.sym == SDLK_RETURN) {
                    handle_login_click(350, 425); // Trigger login/register
                }
            } else if(game.state == STATE_PLACING_SHIPS) {
                if(e.key.keysym.sym == SDLK_r) {
                    game.ship_horizontal = !game.ship_horizontal;
                } else if(e.key.keysym.sym == SDLK_q) {
                    game.state = STATE_LOBBY;
                    send_to_server("GET_USERS#");
                }
            } else if(game.state == STATE_PLAYING) {
                if(e.key.keysym.sym == SDLK_q) {
                    game.state = STATE_LOBBY;
                    send_to_server("GET_USERS#");
                }
            }
        }
    }
}

// ==================== RENDER ====================
void render() {
    pthread_mutex_lock(&game_lock);
    
    if(game.state == STATE_LOGIN) {
        render_login_screen();
    } else if(game.state == STATE_LOBBY) {
        render_lobby_screen();
    } else if(game.state == STATE_WAITING_INVITE || game.state == STATE_RECEIVED_INVITE) {
        render_lobby_screen();  // Render lobby in background
        render_invite_dialog(); // Overlay dialog
    } else if(game.state == STATE_PLACING_SHIPS) {
        render_placing_ships();
    } else if(game.state == STATE_PLAYING) {
        render_playing();
    } else if(game.state == STATE_GAME_OVER) {
        render_playing();  // Show final state
    }
    
    pthread_mutex_unlock(&game_lock);
    SDL_RenderPresent(game.renderer);
}

// ==================== RECEIVE THREAD ====================
void* receive_thread(void* arg) {
    (void)arg;
    char buffer[BUFF_SIZE];
    
    while(game.running) {
        int n = recv(game.sockfd, buffer, BUFF_SIZE-1, 0);
        if(n <= 0) break;
        
        buffer[n] = '\0';
        printf("RECEIVED: %s\n", buffer);
        
        pthread_mutex_lock(&game_lock);
        
        if(strncmp(buffer, "REGISTER_OK", 11) == 0) {
            strcpy(game.login_message, "register successful! Please log in.");
            game.is_register_mode = 0;
        }
        else if(strncmp(buffer, "REGISTER_FAIL", 13) == 0) {
            strcpy(game.login_message, "register failed! Username already exists.");
        }
        else if(strncmp(buffer, "LOGIN_OK:", 9) == 0) {
            // Parse: LOGIN_OK:username:total_games:wins:elo:user_id#
            sscanf(buffer, "LOGIN_OK:%[^:]:%d:%d:%d:%d", 
                   game.my_username, &game.total_games, &game.wins, 
                   &game.my_elo, &game.my_user_id);
            
            game.state = STATE_LOBBY;
            send_to_server("GET_USERS#");
        }
        else if(strncmp(buffer, "LOGIN_FAIL", 10) == 0) {
            strcpy(game.login_message, "login failed! Incorrect username or password.");
        }
        else if(strncmp(buffer, "USERS:", 6) == 0) {
            // Parse user list: USERS:count:id1,name1,status1,elo1:id2,name2,status2,elo2:...#
            char* ptr = buffer + 6;
            sscanf(ptr, "%d:", &game.user_count);
            ptr = strchr(ptr, ':') + 1;
            
            for(int i=0; i<game.user_count; i++) {
                sscanf(ptr, "%d,%[^,],%[^,],%d", 
                       &game.users[i].user_id,
                       game.users[i].username,
                       game.users[i].status,
                       &game.users[i].elo_rating);
                ptr = strchr(ptr + 1, ':');
                if(!ptr) break;
                ptr++;
            }
        }
        else if(strncmp(buffer, "INVITE_FROM:", 12) == 0) {
            // Parse: INVITE_FROM:user_id:username#
            sscanf(buffer, "INVITE_FROM:%d:%[^#]", 
                   &game.inviter_user_id, game.inviter_username);
            game.state = STATE_RECEIVED_INVITE;
        }
        else if(strncmp(buffer, "INVITE_ACCEPTED", 15) == 0) {
            game.state = STATE_PLACING_SHIPS;
            strcpy(game.game_message, "Opponent accepted! Please place your ships.");
        }
        else if(strncmp(buffer, "INVITE_DECLINED", 15) == 0) {
            game.state = STATE_LOBBY;
        }
        else if(strncmp(buffer, "GAME_START", 10) == 0) {
            game.state = STATE_PLACING_SHIPS;
        }
        else if(strncmp(buffer, "START_PLAYING", 13) == 0) {
            game.state = STATE_PLAYING;
            game.is_my_turn = 0; // Server will send who goes first
            snprintf(game.message, sizeof(game.message), "Game started! Good luck!");
        }
        else if(strncmp(buffer, "WAITING_OPPONENT", 16) == 0) {
            snprintf(game.message, sizeof(game.message), "Waiting for opponent to READY...");
        }
        else if(strncmp(buffer, "OPPONENT_READY", 14) == 0) {
            snprintf(game.message, sizeof(game.message), "Opponent is READY! Click READY when done.");
        }
        
        pthread_mutex_unlock(&game_lock);
    }
    
    return NULL;
}

// ==================== MAIN ====================
int main() {
    init_game();
    
    while(game.running) {
        handle_events();
        render();
        SDL_Delay(16); // ~60 FPS
    }
    
    close(game.sockfd);
    TTF_CloseFont(game.font);
    TTF_CloseFont(game.font_small);
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}

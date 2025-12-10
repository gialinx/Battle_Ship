// client_gui_v2.c - Client GUI hoàn chỉnh với màn hình đặt tàu
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "network.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 750
#define CELL_SIZE 40

#define COLOR_OCEAN (SDL_Color){100, 149, 237, 255}
#define COLOR_GRID (SDL_Color){70, 130, 180, 255}
#define COLOR_SHIP (SDL_Color){128, 128, 128, 255}  // Gray for ships
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

typedef enum {
    STATE_PLACING_SHIPS,
    STATE_WAITING_READY,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    TTF_Font *small_font;
    
    int sockfd;
    GameState state;
    
    char own_map[MAP_SIZE][MAP_SIZE];
    char enemy_map[MAP_SIZE][MAP_SIZE];
    
    int is_my_turn;
    int selected_ship_length;
    int ship_horizontal;
    char message[256];
    
    // Đặt tàu
    int ships_to_place[4];  // [4,3,2,2] - số lượng cần đặt mỗi loại
    int ships_placed_count[5]; // count[2], count[3], count[4]
    int mouse_grid_x, mouse_grid_y;
    int preview_valid;
} GameData;

// Forward declarations
void process_server_message(GameData *game, const char *msg);

int init_sdl(GameData *game) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    if(TTF_Init() < 0) return 0;
    
    game->window = SDL_CreateWindow("Battleship Game", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                                     SDL_WINDOW_SHOWN);
    if(!game->window) return 0;
    
    game->renderer = SDL_CreateRenderer(game->window, -1,
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!game->renderer) return 0;
    
    game->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
    game->small_font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if(!game->font || !game->small_font) return 0;
    
    return 1;
}

void init_maps(GameData *game) {
    for(int i = 0; i < MAP_SIZE; i++)
        for(int j = 0; j < MAP_SIZE; j++) {
            game->own_map[i][j] = '-';
            game->enemy_map[i][j] = '-';
        }
}

void draw_text(GameData *game, const char *text, int x, int y, SDL_Color color, TTF_Font *font) {
    SDL_Surface *surface = TTF_RenderUTF8_Solid(font, text, color);
    if(!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(game->renderer, surface);
    if(!texture) { SDL_FreeSurface(surface); return; }
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(game->renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

int draw_button(GameData *game, const char *text, int x, int y, int w, int h, int enabled, int hover) {
    SDL_Rect rect = {x, y, w, h};
    if(!enabled) {
        SDL_SetRenderDrawColor(game->renderer, COLOR_BUTTON_DISABLED.r, COLOR_BUTTON_DISABLED.g, COLOR_BUTTON_DISABLED.b, 255);
    } else if(hover) {
        SDL_SetRenderDrawColor(game->renderer, COLOR_BUTTON_HOVER.r, COLOR_BUTTON_HOVER.g, COLOR_BUTTON_HOVER.b, 255);
    } else {
        SDL_SetRenderDrawColor(game->renderer, COLOR_BUTTON.r, COLOR_BUTTON.g, COLOR_BUTTON.b, 255);
    }
    SDL_RenderFillRect(game->renderer, &rect);
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(game->renderer, &rect);
    
    int text_len = strlen(text);
    draw_text(game, text, x + w/2 - text_len*4, y + h/2 - 10, COLOR_TEXT, game->font);
    return 1;
}

void draw_map(GameData *game, char map[MAP_SIZE][MAP_SIZE], int start_x, int start_y, int is_own_map) {
    for(int i = 0; i < MAP_SIZE; i++) {
        char num[3]; snprintf(num, 3, "%d", i + 1);
        draw_text(game, num, start_x + i * CELL_SIZE + CELL_SIZE/2 - 5, start_y - 25, COLOR_TEXT, game->small_font);
        draw_text(game, num, start_x - 30, start_y + i * CELL_SIZE + CELL_SIZE/2 - 8, COLOR_TEXT, game->small_font);
    }
    
    for(int row = 0; row < MAP_SIZE; row++) {
        for(int col = 0; col < MAP_SIZE; col++) {
            SDL_Rect rect = {start_x + col * CELL_SIZE, start_y + row * CELL_SIZE, CELL_SIZE - 2, CELL_SIZE - 2};
            char cell = map[row][col];
            
            // Debug: print non-empty cells
            if(cell != '-' && is_own_map && row < 5) {
                printf("DEBUG RENDER: Cell[%d][%d] = '%c' (ASCII %d)\n", row, col, cell, (int)cell);
            }
            
            if(cell == '-') {
                SDL_SetRenderDrawColor(game->renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            } else if(cell >= '2' && cell <= '9') {
                printf("DEBUG: Drawing GRAY ship at [%d][%d] cell='%c'\n", row, col, cell);
                if(is_own_map) SDL_SetRenderDrawColor(game->renderer, COLOR_SHIP.r, COLOR_SHIP.g, COLOR_SHIP.b, 255);
                else SDL_SetRenderDrawColor(game->renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            } else if(cell == 'x') {
                SDL_SetRenderDrawColor(game->renderer, COLOR_MISS.r, COLOR_MISS.g, COLOR_MISS.b, 255);
            } else if(cell == 'o') {
                SDL_SetRenderDrawColor(game->renderer, COLOR_HIT.r, COLOR_HIT.g, COLOR_HIT.b, 255);
            } else if(cell == '@') {
                SDL_SetRenderDrawColor(game->renderer, COLOR_SUNK.r, COLOR_SUNK.g, COLOR_SUNK.b, 255);
            } else {
                SDL_SetRenderDrawColor(game->renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            }
            
            SDL_RenderFillRect(game->renderer, &rect);
            SDL_SetRenderDrawColor(game->renderer, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, 255);
            SDL_RenderDrawRect(game->renderer, &rect);
            
            if(cell == 'x') {
                SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(game->renderer, rect.x + 5, rect.y + 5, rect.x + rect.w - 5, rect.y + rect.h - 5);
                SDL_RenderDrawLine(game->renderer, rect.x + rect.w - 5, rect.y + 5, rect.x + 5, rect.y + rect.h - 5);
            } else if(cell == 'o' || cell == '@') {
                SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255);
                int cx = rect.x + rect.w/2, cy = rect.y + rect.h/2;
                for(int r = 8; r <= 12; r++) {
                    for(int angle = 0; angle < 360; angle += 10) {
                        SDL_RenderDrawPoint(game->renderer, cx + r * SDL_cos(angle * M_PI / 180), cy + r * SDL_sin(angle * M_PI / 180));
                    }
                }
            }
        }
    }
}

int check_placement_valid(GameData *game, int x, int y, int length, int horizontal) {
    if(x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE) return 0;
    
    int dx = horizontal ? 1 : 0;
    int dy = horizontal ? 0 : 1;
    int ex = x + dx * (length - 1);
    int ey = y + dy * (length - 1);
    
    if(ex >= MAP_SIZE || ey >= MAP_SIZE) return 0;
    
    for(int i = 0; i < length; i++) {
        int tx = x + dx * i;
        int ty = y + dy * i;
        if(game->own_map[ty][tx] != '-') return 0;
    }
    return 1;
}

void draw_ship_preview(GameData *game, int start_x, int start_y) {
    if(game->selected_ship_length == 0) return;
    if(game->mouse_grid_x < 0 || game->mouse_grid_y < 0) return;
    
    int length = game->selected_ship_length;
    int x = game->mouse_grid_x;
    int y = game->mouse_grid_y;
    int horizontal = game->ship_horizontal;
    
    int valid = check_placement_valid(game, x, y, length, horizontal);
    game->preview_valid = valid;
    
    SDL_Color color = valid ? COLOR_SHIP_PREVIEW : COLOR_SHIP_INVALID;
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);
    
    for(int i = 0; i < length; i++) {
        int tx = x + (horizontal ? i : 0);
        int ty = y + (horizontal ? 0 : i);
        if(tx >= 0 && tx < MAP_SIZE && ty >= 0 && ty < MAP_SIZE) {
            SDL_Rect rect = {start_x + tx * CELL_SIZE, start_y + ty * CELL_SIZE, CELL_SIZE - 2, CELL_SIZE - 2};
            SDL_SetRenderDrawColor(game->renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(game->renderer, &rect);
        }
    }
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_NONE);
}

void render_placing_ships(GameData *game) {
    SDL_SetRenderDrawColor(game->renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(game->renderer);
    
    draw_text(game, "DAT TAU - BATTLESHIP", 450, 10, COLOR_TEXT, game->font);
    draw_text(game, "Nhan [R] de xoay tau, Click chuot de dat", 350, 40, (SDL_Color){200,200,200,255}, game->small_font);
    
    // Danh sách tàu
    int list_x = 50, list_y = 80;
    draw_text(game, "DANH SACH TAU:", list_x, list_y, COLOR_TEXT, game->font);
    
    const char *ship_names[] = {"Tau lon (4 o)", "Tau vua (3 o)", "Tau nho (2 o)", "Tau nho (2 o)"};
    int ship_lengths[] = {4, 3, 2, 2};
    
    for(int i = 0; i < 4; i++) {
        int y = list_y + 40 + i * 60;
        int placed = (ship_lengths[i] == 4 && game->ships_placed_count[4] > 0) ||
                     (ship_lengths[i] == 3 && game->ships_placed_count[3] > 0) ||
                     (ship_lengths[i] == 2 && game->ships_placed_count[2] >= (i == 2 ? 1 : 2));
        
        SDL_Color btn_color = placed ? (SDL_Color){0,200,0,255} : (SDL_Color){200,200,200,255};
        char btn_text[64];
        snprintf(btn_text, 64, "%s %s", ship_names[i], placed ? "[OK]" : "");
        
        int hover = 0;
        draw_button(game, btn_text, list_x, y, 180, 40, !placed, hover);
        
        if(game->selected_ship_length == ship_lengths[i] && !placed) {
            SDL_Rect rect = {list_x - 5, y - 5, 190, 50};
            SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(game->renderer, &rect);
        }
    }
    
    // Vẽ bản đồ
    int map_x = 300, map_y = 100;
    draw_text(game, "BAN DO CUA BAN", map_x + 150, map_y - 30, COLOR_TEXT, game->font);
    draw_map(game, game->own_map, map_x, map_y, 1);
    draw_ship_preview(game, map_x, map_y);
    
    // Button READY
    int all_placed = (game->ships_placed_count[4] >= 1 && game->ships_placed_count[3] >= 1 && game->ships_placed_count[2] >= 2);
    draw_button(game, "READY", WINDOW_WIDTH - 200, WINDOW_HEIGHT - 80, 150, 50, all_placed, 0);
    
    // Message
    draw_text(game, game->message, 50, WINDOW_HEIGHT - 50, (SDL_Color){255,255,0,255}, game->font);
    
    SDL_RenderPresent(game->renderer);
}

void render_playing(GameData *game) {
    SDL_SetRenderDrawColor(game->renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(game->renderer);
    
    draw_text(game, "BATTLESHIP - TRAN DAU", 450, 10, COLOR_TEXT, game->font);
    
    if(game->is_my_turn) {
        draw_text(game, "LUOT CUA BAN!", 500, 40, (SDL_Color){0, 255, 0, 255}, game->font);
    } else {
        draw_text(game, "Cho doi thu...", 500, 40, (SDL_Color){255, 255, 0, 255}, game->font);
    }
    
    draw_text(game, "BAN DO CUA BAN", 120, 70, COLOR_TEXT, game->font);
    draw_text(game, "BAN DO DOI THU", 680, 70, COLOR_TEXT, game->font);
    
    draw_map(game, game->own_map, 50, 100, 1);
    draw_map(game, game->enemy_map, 610, 100, 0);
    
    if(strlen(game->message) > 0) {
        draw_text(game, game->message, 50, 650, (SDL_Color){255, 255, 0, 255}, game->font);
    }
    
    SDL_RenderPresent(game->renderer);
}

void place_ship(GameData *game, int x, int y) {
    if(!game->preview_valid) {
        snprintf(game->message, sizeof(game->message), "Khong the dat tau o day!");
        return;
    }
    
    int length = game->selected_ship_length;
    char dir = game->ship_horizontal ? 'H' : 'V';
    
    // Gửi lệnh PLACE tới server
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "PLACE:%d,%d,%d,%c", length, x + 1, y + 1, dir);
    send_command(game->sockfd, cmd);
    
    // Đợi và nhận nhiều lần để đảm bảo nhận được STATE
    for(int attempt = 0; attempt < 5; attempt++) {
        usleep(100000); // 100ms mỗi lần
        char buffer[BUFF_SIZE];
        int n = receive_data(game->sockfd, buffer, sizeof(buffer));
        if(n > 0) {
            buffer[n] = '\0';
            char *token = strtok(buffer, "#");
            while(token) {
                process_server_message(game, token);
                token = strtok(NULL, "#");
            }
        }
    }
    
    // Đếm lại tàu từ bản đồ
    int total_ships = game->ships_placed_count[4] + game->ships_placed_count[3] + game->ships_placed_count[2];
    
    snprintf(game->message, sizeof(game->message), "Da dat tau %d o! (%d/4 tau)", 
             length, total_ships);
    
    // Reset lựa chọn sau khi đặt xong
    game->selected_ship_length = 0;
    
    // Kiểm tra đã đặt đủ chưa
    int all_placed = (game->ships_placed_count[4] >= 1 && game->ships_placed_count[3] >= 1 && game->ships_placed_count[2] >= 2);
    if(all_placed) {
        snprintf(game->message, sizeof(game->message), "DA DAT DU 4 TAU! Click nut READY de bat dau!");
    }
    
    printf("DEBUG: Ships placed - 4:%d 3:%d 2:%d\n", 
           game->ships_placed_count[4], game->ships_placed_count[3], game->ships_placed_count[2]);
}

void handle_click_placing_ships(GameData *game, int mouse_x, int mouse_y) {
    // Chọn tàu từ danh sách
    int list_x = 50, list_y = 120;
    int ship_lengths[] = {4, 3, 2, 2};
    
    for(int i = 0; i < 4; i++) {
        int y = list_y + i * 60;
        SDL_Rect btn = {list_x, y, 180, 40};
        if(mouse_x >= btn.x && mouse_x <= btn.x + btn.w && mouse_y >= btn.y && mouse_y <= btn.y + btn.h) {
            int placed = (ship_lengths[i] == 4 && game->ships_placed_count[4] > 0) ||
                         (ship_lengths[i] == 3 && game->ships_placed_count[3] > 0) ||
                         (ship_lengths[i] == 2 && game->ships_placed_count[2] >= (i == 2 ? 1 : 2));
            if(!placed) {
                game->selected_ship_length = ship_lengths[i];
                snprintf(game->message, sizeof(game->message), "Da chon tau %d o", ship_lengths[i]);
            }
            return;
        }
    }
    
    // Click vào bản đồ để đặt tàu
    int map_x = 300, map_y = 100;
    if(mouse_x >= map_x && mouse_x < map_x + MAP_SIZE * CELL_SIZE &&
       mouse_y >= map_y && mouse_y < map_y + MAP_SIZE * CELL_SIZE) {
        int grid_x = (mouse_x - map_x) / CELL_SIZE;
        int grid_y = (mouse_y - map_y) / CELL_SIZE;
        if(game->selected_ship_length > 0) {
            place_ship(game, grid_x, grid_y);
        }
    }
    
    // Click READY
    SDL_Rect ready_btn = {WINDOW_WIDTH - 200, WINDOW_HEIGHT - 80, 150, 50};
    if(mouse_x >= ready_btn.x && mouse_x <= ready_btn.x + ready_btn.w &&
       mouse_y >= ready_btn.y && mouse_y <= ready_btn.y + ready_btn.h) {
        int all_placed = (game->ships_placed_count[4] >= 1 && game->ships_placed_count[3] >= 1 && game->ships_placed_count[2] >= 2);
        if(all_placed) {
            send_command(game->sockfd, "READY");
            game->state = STATE_WAITING_READY;
            snprintf(game->message, sizeof(game->message), "Cho doi thu READY...");
        }
    }
}

void handle_click_playing(GameData *game, int mouse_x, int mouse_y) {
    if(!game->is_my_turn) return;
    
    int start_x = 610, start_y = 100;
    if(mouse_x < start_x || mouse_x > start_x + MAP_SIZE * CELL_SIZE) return;
    if(mouse_y < start_y || mouse_y > start_y + MAP_SIZE * CELL_SIZE) return;
    
    int col = (mouse_x - start_x) / CELL_SIZE;
    int row = (mouse_y - start_y) / CELL_SIZE;
    if(col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return;
    
    char cell = game->enemy_map[row][col];
    if(cell == 'x' || cell == 'o' || cell == '@') {
        snprintf(game->message, sizeof(game->message), "Da ban o nay roi!");
        return;
    }
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "FIRE:%d,%d", col + 1, row + 1);
    send_command(game->sockfd, cmd);
    snprintf(game->message, sizeof(game->message), "Ban vao (%d,%d)...", col + 1, row + 1);
}

void parse_state(GameData *game, const char *state_data) {
    const char *own_start = strstr(state_data, "OWN MAP");
    const char *enemy_start = strstr(state_data, "ENEMY MAP");
    if(!own_start || !enemy_start) {
        printf("DEBUG: Failed to find OWN MAP or ENEMY MAP in STATE\n");
        return;
    }
    
    // Skip "OWN MAP\n" to get to the first row
    own_start = strchr(own_start, '\n');
    if(!own_start) return;
    own_start++; // Skip the newline
    
    // Skip "ENEMY MAP\n" 
    enemy_start = strchr(enemy_start, '\n');
    if(!enemy_start) return;
    enemy_start++;
    
    // Find where ENEMY MAP starts to know where OWN MAP ends
    const char *enemy_marker = strstr(own_start, "ENEMY MAP");
    if(!enemy_marker) return;
    
    char own_copy[BUFF_SIZE];
    int copy_len = enemy_marker - own_start;
    if(copy_len >= BUFF_SIZE) copy_len = BUFF_SIZE - 1;
    strncpy(own_copy, own_start, copy_len);
    own_copy[copy_len] = '\0';
    
    // Parse OWN MAP row by row (each row separated by \n)
    char *row_tok = strtok(own_copy, "\n");
    int row = 0;
    printf("DEBUG: Parsing OWN MAP:\n");
    while(row_tok && row < MAP_SIZE) {
        printf("Row %d: [%s]\n", row, row_tok);
        
        // Parse each character (skip spaces)
        int col = 0;
        for(int i = 0; row_tok[i] != '\0' && col < MAP_SIZE; i++) {
            if(row_tok[i] != ' ') {
                game->own_map[row][col] = row_tok[i];
                col++;
            }
        }
        row++;
        row_tok = strtok(NULL, "\n");
    }
    
    // Print the map for debugging
    printf("DEBUG: Own map after parsing:\n");
    for(int r = 0; r < 5; r++) {
        printf("  ");
        for(int c = 0; c < MAP_SIZE; c++) {
            printf("%c ", game->own_map[r][c]);
        }
        printf("\n");
    }
    
    // DEBUG: Print map to console
    printf("DEBUG: Own map after STATE update:\n");
    for(int r = 0; r < MAP_SIZE; r++) {
        printf("  Row %2d: ", r);
        for(int c = 0; c < MAP_SIZE; c++) {
            printf("%c ", game->own_map[r][c]);
        }
        printf("\n");
    }
    
    // Đếm tàu đã đặt từ bản đồ - đếm đơn giản bằng cách đếm ký tự
    for(int i = 2; i <= 4; i++) game->ships_placed_count[i] = 0;
    
    int visited[MAP_SIZE][MAP_SIZE] = {0};
    for(int r = 0; r < MAP_SIZE; r++) {
        for(int c = 0; c < MAP_SIZE; c++) {
            char ch = game->own_map[r][c];
            if(ch >= '2' && ch <= '9' && !visited[r][c]) {
                // Đếm một tàu mới
                int length = ch - '0';
                
                // Đánh dấu tất cả ô của tàu này
                // Thử ngang
                int ship_len = 0;
                for(int cc = c; cc < MAP_SIZE && game->own_map[r][cc] == ch; cc++) {
                    visited[r][cc] = 1;
                    ship_len++;
                }
                
                // Nếu ship_len vẫn là 0, thử dọc
                if(ship_len == 0) {
                    for(int rr = r; rr < MAP_SIZE && game->own_map[rr][c] == ch; rr++) {
                        visited[rr][c] = 1;
                        ship_len++;
                    }
                }
                
                // Đếm tàu dựa trên ký tự (length từ server)
                if(length >= 2 && length <= 4) {
                    game->ships_placed_count[length]++;
                    printf("DEBUG: Found ship length %d at (%d,%d), count now: %d\n", 
                           length, c, r, game->ships_placed_count[length]);
                }
            }
        }
    }
    
    // Parse ENEMY MAP
    char enemy_copy[BUFF_SIZE];
    const char *enemy_end = strchr(enemy_start, '#');
    if(enemy_end) {
        int len = enemy_end - enemy_start;
        if(len >= BUFF_SIZE) len = BUFF_SIZE - 1;
        strncpy(enemy_copy, enemy_start, len);
        enemy_copy[len] = '\0';
    } else {
        strncpy(enemy_copy, enemy_start, BUFF_SIZE - 1);
        enemy_copy[BUFF_SIZE - 1] = '\0';
    }
    
    row_tok = strtok(enemy_copy, "\n");
    row = 0;
    while(row_tok && row < MAP_SIZE) {
        int col = 0;
        for(int i = 0; row_tok[i] != '\0' && col < MAP_SIZE; i++) {
            if(row_tok[i] != ' ') {
                game->enemy_map[row][col] = row_tok[i];
                col++;
            }
        }
        row++;
        row_tok = strtok(NULL, "\n");
    }
}

void process_server_message(GameData *game, const char *msg) {
    printf("Server: %s\n", msg);
    
    if(strncmp(msg, "STATE:", 6) == 0) {
        parse_state(game, msg);
    } else if(strcmp(msg, "YOUR_TURN:") == 0) {
        game->is_my_turn = 1;
        game->state = STATE_PLAYING;
        snprintf(game->message, sizeof(game->message), "Den luot ban!");
    } else if(strcmp(msg, "WAIT_YOUR_TURN:") == 0) {
        game->is_my_turn = 0;
        game->state = STATE_PLAYING;
        snprintf(game->message, sizeof(game->message), "Doi thu dang ban...");
    } else if(strcmp(msg, "HIT_CONTINUE:") == 0) {
        snprintf(game->message, sizeof(game->message), "TRUNG! Ban tiep!");
    } else if(strncmp(msg, "YOU WIN:", 8) == 0) {
        game->state = STATE_GAME_OVER;
        snprintf(game->message, sizeof(game->message), "BAN THANG!");
    } else if(strncmp(msg, "YOU LOSE:", 9) == 0) {
        game->state = STATE_GAME_OVER;
        snprintf(game->message, sizeof(game->message), "BAN THUA!");
    } else if(strncmp(msg, "MATCH_START:", 12) == 0) {
        printf("DEBUG: MATCH_START received! Changing to STATE_PLAYING\n");
        game->state = STATE_PLAYING;
        snprintf(game->message, sizeof(game->message), "Tran dau bat dau!");
    } else if(strncmp(msg, "PLACE_OK:", 9) == 0) {
        snprintf(game->message, sizeof(game->message), "Dat tau thanh cong!");
    } else if(strcmp(msg, "READY_OK:") == 0) {
        printf("DEBUG: READY_OK received, waiting for opponent\n");
        snprintf(game->message, sizeof(game->message), "READY OK! Cho doi thu...");
    } else if(strstr(msg, "Waiting for opponent")) {
        printf("DEBUG: Server says waiting for opponent\n");
        snprintf(game->message, sizeof(game->message), "CHO DOI THU READY...");
    }
}

void receive_server_data(GameData *game) {
    static int receive_count = 0;
    static int last_state = -1;
    char buffer[BUFF_SIZE];
    int n = receive_data(game->sockfd, buffer, sizeof(buffer));
    receive_count++;
    
    // Debug state changes
    if(game->state != last_state) {
        printf("DEBUG: State changed from %d to %d\n", last_state, game->state);
        last_state = game->state;
    }
    
    if(n > 0) {
        buffer[n] = '\0';
        printf("DEBUG: [#%d] Socket %d received %d bytes: [%s]\n", receive_count, game->sockfd, n, buffer);
        char *token = strtok(buffer, "#");
        while(token) {
            printf("DEBUG: [#%d] Processing token: [%s]\n", receive_count, token);
            process_server_message(game, token);
            token = strtok(NULL, "#");
        }
    } else if(n < 0) {
        // Non-blocking socket, no data available yet
        if(receive_count % 5000 == 0) {
            printf("DEBUG: [#%d] No data after %d attempts, state=%d\n", receive_count, receive_count, game->state);
        }
    } else {
        // n == 0 means connection closed
        printf("DEBUG: Connection closed by server\n");
    }
}

void game_loop(GameData *game) {
    int running = 1;
    SDL_Event event;
    
    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = 0;
            } else if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if(game->state == STATE_PLACING_SHIPS) {
                    handle_click_placing_ships(game, event.button.x, event.button.y);
                } else if(game->state == STATE_PLAYING) {
                    handle_click_playing(game, event.button.x, event.button.y);
                }
            } else if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.sym == SDLK_r && game->state == STATE_PLACING_SHIPS) {
                    game->ship_horizontal = !game->ship_horizontal;
                    snprintf(game->message, sizeof(game->message), "Huong: %s", game->ship_horizontal ? "Ngang" : "Doc");
                }
            } else if(event.type == SDL_MOUSEMOTION && game->state == STATE_PLACING_SHIPS) {
                int map_x = 300, map_y = 100;
                game->mouse_grid_x = (event.motion.x - map_x) / CELL_SIZE;
                game->mouse_grid_y = (event.motion.y - map_y) / CELL_SIZE;
            }
        }
        
        // Try receiving multiple times to catch all pending messages
        for(int i = 0; i < 3; i++) {
            receive_server_data(game);
        }
        
        if(game->state == STATE_PLACING_SHIPS) {
            render_placing_ships(game);
        } else if(game->state == STATE_WAITING_READY) {
            SDL_SetRenderDrawColor(game->renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
            SDL_RenderClear(game->renderer);
            draw_text(game, "CHO DOI THU READY...", 450, 300, COLOR_TEXT, game->font);
            SDL_RenderPresent(game->renderer);
        } else if(game->state == STATE_PLAYING || game->state == STATE_GAME_OVER) {
            render_playing(game);
        }
        
        SDL_Delay(16);
    }
}

void cleanup(GameData *game) {
    if(game->font) TTF_CloseFont(game->font);
    if(game->small_font) TTF_CloseFont(game->small_font);
    if(game->renderer) SDL_DestroyRenderer(game->renderer);
    if(game->window) SDL_DestroyWindow(game->window);
    if(game->sockfd > 0) close(game->sockfd);
    TTF_Quit();
    SDL_Quit();
}

int main() {
    GameData game = {0};
    
    if(!init_sdl(&game)) return 1;
    
    init_maps(&game);
    game.state = STATE_PLACING_SHIPS;
    game.selected_ship_length = 0;
    game.ship_horizontal = 1;
    game.mouse_grid_x = -1;
    game.mouse_grid_y = -1;
    strcpy(game.message, "Chon tau va dat len ban do");
    
    game.sockfd = connect_to_server("127.0.0.1", PORT);
    if(game.sockfd < 0) {
        fprintf(stderr, "Khong the ket noi server!\n");
        cleanup(&game);
        return 1;
    }
    
    printf("Da ket noi toi server!\n");
    
    game_loop(&game);
    cleanup(&game);
    return 0;
}

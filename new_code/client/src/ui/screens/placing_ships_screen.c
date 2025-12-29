// placing_ships_screen.c - Ship placement screen implementation
#include "placing_ships_screen.h"
#include "../../ui/renderer.h"
#include "../../ui/colors.h"
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define MAP_X 200
#define MAP_Y 100
#define CELL_DISPLAY 28
#define LIST_X 20
#define LIST_Y 70

// ==================== INITIALIZE ====================
void placing_ships_init(GameData* game) {
    game->selected_ship_length = 0;
    game->selected_ship_id = -1;
    game->ship_horizontal = 1;
    game->ships_to_place[0] = 4;
    game->ships_to_place[1] = 3;
    game->ships_to_place[2] = 2;
    game->ships_to_place[3] = 2;
    
    for(int i = 0; i < 5; i++) {
        game->ships_placed_count[i] = 0;
    }
    game->ships_placed_countMax[2] = 2;
    game->ships_placed_countMax[3] = 1;
    game->ships_placed_countMax[4] = 1;
    
    game->mouse_grid_x = -1;
    game->mouse_grid_y = -1;
    game->preview_valid = 0;
    strcpy(game->message, "Chon tau va dat len ban do");
}

// ==================== CHECK VALID ====================
int placing_ships_check_valid(GameData* game, int x, int y, int length, int horizontal) {
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

// ==================== CHECK COMPLETE ====================
int placing_ships_check_complete(GameData* game) {
    return (game->ships_placed_count[4] >= 1 && 
            game->ships_placed_count[3] >= 1 && 
            game->ships_placed_count[2] >= 2);
}

// ==================== RENDER ====================
void placing_ships_render(SDL_Renderer* renderer, GameData* game) {
    SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(renderer);
    
    // Title
    render_text(renderer, game->font_small, "DAT TAU - BATTLESHIP", 320, 10, COLOR_TEXT);
    render_text(renderer, game->font_small, "Nhan [R] de xoay tau, Click chuot de dat", 
                250, 35, (SDL_Color){180,180,180,255});
    
    // Ship list
    render_text(renderer, game->font_small, "CHON TAU:", LIST_X, LIST_Y, COLOR_TEXT);
    
    const char *ship_names[] = {"Lon (4o)", "Vua (3o)", "Nho (2o)"};
    int ship_lengths[] = {4, 3, 2};
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    for(int i = 0; i < 3; i++) {
        int y = LIST_Y + 30 + i * 55;
        int max_count = (ship_lengths[i] == 2) ? 2 : 1;
        int placed_count = game->ships_placed_count[ship_lengths[i]];
        int placed = (placed_count >= max_count);
        
        char btn_text[64];
        snprintf(btn_text, sizeof(btn_text), "%s [%d/%d]", ship_names[i], placed_count, max_count);
        
        int hover = (mx >= LIST_X && mx <= LIST_X + 150 && my >= y && my <= y + 40);
        render_button(renderer, game->font_small, btn_text, LIST_X, y, 150, 40, 
                    COLOR_BUTTON, hover, !placed);
        
        // Yellow border for selected ship
        if(game->selected_ship_length == ship_lengths[i] && !placed && game->selected_ship_id == i) {
            SDL_Rect rect = {LIST_X - 3, y - 3, 156, 46};
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            for(int w = 0; w < 3; w++) {
                SDL_Rect border = {rect.x - w, rect.y - w, rect.w + w*2, rect.h + w*2};
                SDL_RenderDrawRect(renderer, &border);
            }
        }
        
        // Green checkmark for completed
        if(placed) {
            render_text(renderer, game->font_small, "✓", LIST_X + 120, y + 10, 
                       (SDL_Color){0,255,0,255});
        }
    }
    
    // Instructions
    int info_y = LIST_Y + 200;
    render_text(renderer, game->font_small, "HUONG DAN:", LIST_X, info_y, 
               (SDL_Color){255,200,100,255});
    render_text(renderer, game->font_small, "1. Chon tau", LIST_X, info_y + 25, 
               (SDL_Color){200,200,200,255});
    render_text(renderer, game->font_small, "2. Nhan R xoay", LIST_X, info_y + 45, 
               (SDL_Color){200,200,200,255});
    render_text(renderer, game->font_small, "3. Click dat", LIST_X, info_y + 65, 
               (SDL_Color){200,200,200,255});
    render_text(renderer, game->font_small, "4. READY choi", LIST_X, info_y + 85, 
               (SDL_Color){200,200,200,255});
    
    // Draw map
    render_text(renderer, game->font_small, "BAN DO CUA BAN", MAP_X + 140, MAP_Y - 30, COLOR_TEXT);
    
    // Draw custom smaller map
    for(int row = 0; row < MAP_SIZE; row++) {
        for(int col = 0; col < MAP_SIZE; col++) {
            SDL_Rect rect = {MAP_X + col * CELL_DISPLAY, MAP_Y + row * CELL_DISPLAY, 
                           CELL_DISPLAY - 2, CELL_DISPLAY - 2};
            char cell = game->own_map[row][col];
            
            if(cell == '-') {
                SDL_SetRenderDrawColor(renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            } else if(cell >= '2' && cell <= '9') {
                SDL_SetRenderDrawColor(renderer, COLOR_SHIP.r, COLOR_SHIP.g, COLOR_SHIP.b, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, 255);
            }
            
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
    
    // Draw ship preview
    if(game->selected_ship_length > 0 && game->mouse_grid_x >= 0 && game->mouse_grid_y >= 0) {
        int valid = placing_ships_check_valid(game, game->mouse_grid_x, game->mouse_grid_y,
                                             game->selected_ship_length, game->ship_horizontal);
        game->preview_valid = valid;
        
        render_ship_preview(renderer, MAP_X, MAP_Y, CELL_DISPLAY,
                          game->mouse_grid_x, game->mouse_grid_y,
                          game->selected_ship_length, game->ship_horizontal,
                          valid, game->own_map);
    }
    
    // READY button
    int all_placed = placing_ships_check_complete(game);
    int ready_x = MAP_X + 150, ready_y = MAP_Y + MAP_SIZE * CELL_DISPLAY + 20;
    int ready_hover = (mx >= ready_x && mx <= ready_x + 150 && my >= ready_y && my <= ready_y + 50);

    if(all_placed) {
        render_button(renderer, game->font_small, "READY!", ready_x, ready_y, 150, 50,
                     COLOR_BUTTON, ready_hover, 1);
    } else {
        render_button(renderer, game->font_small, "Can dat 4 tau", ready_x, ready_y, 150, 50,
                     COLOR_BUTTON_DISABLED, 0, 0);
    }

    // BACK button
    int back_x = MAP_X - 40, back_y = MAP_Y + MAP_SIZE * CELL_DISPLAY + 20;
    int back_hover = (mx >= back_x && mx <= back_x + 150 && my >= back_y && my <= back_y + 50);
    render_button(renderer, game->font_small, "< Back to Lobby", back_x, back_y, 150, 50,
                 (SDL_Color){200, 100, 0, 255}, back_hover, 1);
    
    // Message
    if(strlen(game->message) > 0) {
        render_text(renderer, game->font_small, game->message, 200, 650, 
                   (SDL_Color){255,255,100,255});
    }
    
    // Status counter
    char status[128];
    snprintf(status, sizeof(status), "Da dat: %d/4 tau", 
             game->ships_placed_count[4] + game->ships_placed_count[3] + game->ships_placed_count[2]);
    render_text(renderer, game->font_small, status, 200, 630, (SDL_Color){100,200,255,255});
}

// ==================== HANDLE CLICK ====================
void placing_ships_handle_click(GameData* game, int x, int y) {
    // Select ship from list
    int ship_lengths[] = {4, 3, 2};
    
    for(int i = 0; i < 3; i++) {
        int btn_y = LIST_Y + 30 + i * 55;
        if(x >= LIST_X && x <= LIST_X + 150 && y >= btn_y && y <= btn_y + 40) {
            int max_count = (ship_lengths[i] == 2) ? 2 : 1;
            int placed_count = game->ships_placed_count[ship_lengths[i]];
            if(placed_count < max_count) {
                game->selected_ship_length = ship_lengths[i];
                game->selected_ship_id = i;
                snprintf(game->message, sizeof(game->message), "Da chon tau %d o", ship_lengths[i]);
            }
            return;
        }
    }
    
    // Click on map to place ship
    if(x >= MAP_X && x < MAP_X + MAP_SIZE * CELL_DISPLAY &&
       y >= MAP_Y && y < MAP_Y + MAP_SIZE * CELL_DISPLAY) {
        int grid_x = (x - MAP_X) / CELL_DISPLAY;
        int grid_y = (y - MAP_Y) / CELL_DISPLAY;
        
        if(game->selected_ship_length > 0 && game->preview_valid) {
            int length = game->selected_ship_length;
            int horizontal = game->ship_horizontal;
            
            // Mark on map
            for(int i = 0; i < length; i++) {
                int tx = grid_x + (horizontal ? i : 0);
                int ty = grid_y + (horizontal ? 0 : i);
                game->own_map[ty][tx] = '0' + length; // '2', '3', or '4'
            }
            
            game->ships_placed_count[length]++;
            snprintf(game->message, sizeof(game->message), "Da dat tau %d o! (%d/%d)", 
                     length, game->ships_placed_count[length], (length == 2 ? 2 : 1));
            
            game->selected_ship_length = 0;
            game->selected_ship_id = -1;
        }
    }
    
    // Click READY button
    int ready_x = MAP_X + 150, ready_y = MAP_Y + MAP_SIZE * CELL_DISPLAY + 20;
    if(x >= ready_x && x <= ready_x + 150 && y >= ready_y && y <= ready_y + 50) {
        if(placing_ships_check_complete(game)) {
            // Will be handled by main client
            snprintf(game->message, sizeof(game->message), "Cho doi thu READY...");
        }
    }

    // Click BACK button
    int back_x = MAP_X - 40, back_y = MAP_Y + MAP_SIZE * CELL_DISPLAY + 20;
    if(x >= back_x && x <= back_x + 150 && y >= back_y && y <= back_y + 50) {
        // Reset game state and return to lobby
        game->state = STATE_LOBBY;
        for(int i=0; i<MAP_SIZE; i++)
            for(int j=0; j<MAP_SIZE; j++)
                game->own_map[i][j] = '-';
        placing_ships_init(game);
    }
}

// ==================== HANDLE KEY ====================
void placing_ships_handle_key(GameData* game, SDL_Keycode key) {
    if(key == SDLK_r) {
        game->ship_horizontal = !game->ship_horizontal;
        snprintf(game->message, sizeof(game->message), "Huong: %s", 
                 game->ship_horizontal ? "Ngang" : "Doc");
    }
}

// ==================== HANDLE MOTION ====================
void placing_ships_handle_motion(GameData* game, int x, int y) {
    if(x >= MAP_X && x < MAP_X + MAP_SIZE * CELL_DISPLAY &&
       y >= MAP_Y && y < MAP_Y + MAP_SIZE * CELL_DISPLAY) {
        game->mouse_grid_x = (x - MAP_X) / CELL_DISPLAY;
        game->mouse_grid_y = (y - MAP_Y) / CELL_DISPLAY;
    } else {
        game->mouse_grid_x = -1;
        game->mouse_grid_y = -1;
    }
}


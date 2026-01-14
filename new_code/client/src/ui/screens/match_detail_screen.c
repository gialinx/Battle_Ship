// match_detail_screen.c - Detailed shot-by-shot replay with scrolling
#include "match_detail_screen.h"
#include "../colors.h"
#include "../renderer.h"
#include <stdio.h>
#include <string.h>

#define HEADER_HEIGHT 80
#define BACK_BTN_X 50
#define BACK_BTN_Y 20
#define BACK_BTN_W 150
#define BACK_BTN_H 40
#define SHIP_MAP_HEIGHT 300
#define SHOT_LIST_Y (HEADER_HEIGHT + SHIP_MAP_HEIGHT + 20)
#define SHOT_LIST_HEIGHT 280
#define LINE_HEIGHT 20

// Scroll state
static int scroll_offset = 0;
static int max_scroll = 0;

void match_detail_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_Rect bg = {0, 0, 1000, 700};
    SDL_RenderFillRect(renderer, &bg);
    
    // Colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {150, 150, 150, 255};
    SDL_Color green = {50, 255, 100, 255};
    SDL_Color red = {255, 100, 100, 255};
    SDL_Color cyan = {100, 200, 255, 255};
    SDL_Color yellow = {255, 220, 80, 255};
    SDL_Color dark_bg = {30, 45, 70, 255};
    
    MatchDetail* detail = &game->current_match_detail;
    
    // Header
    SDL_SetRenderDrawColor(renderer, dark_bg.r, dark_bg.g, dark_bg.b, 255);
    SDL_Rect header = {0, 0, 1000, HEADER_HEIGHT};
    SDL_RenderFillRect(renderer, &header);
    
    char title[256];
    snprintf(title, sizeof(title), "MATCH DETAIL #%d", detail->match_id);
    render_text(renderer, game->font, title, 350, 15, white);
    
    char vs_text[256];
    snprintf(vs_text, sizeof(vs_text), "%s vs %s", detail->my_name, detail->opponent_name);
    render_text(renderer, game->font_small, vs_text, 350, 50, cyan);
    
    // Back button
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int back_hover = (mx >= BACK_BTN_X && mx <= BACK_BTN_X + BACK_BTN_W &&
                      my >= BACK_BTN_Y && my <= BACK_BTN_Y + BACK_BTN_H);
    
    SDL_Rect back_btn = {BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H};
    SDL_SetRenderDrawColor(renderer, back_hover ? 80 : 60, back_hover ? 100 : 80, back_hover ? 120 : 100, 255);
    SDL_RenderFillRect(renderer, &back_btn);
    SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
    SDL_RenderDrawRect(renderer, &back_btn);
    render_text_centered(renderer, game->font_small, "< BACK", BACK_BTN_X + BACK_BTN_W/2, BACK_BTN_Y + 12, white);
    
    // Ship maps section
    int map_y = HEADER_HEIGHT + 10;
    int cell_size = 20;
    int map_start_x = 100;
    
    // My ships
    render_text(renderer, game->font_small, "YOUR SHIPS:", map_start_x, map_y, cyan);
    int my_map_y = map_y + 25;
    for(int r = 0; r < MAP_SIZE; r++) {
        for(int c = 0; c < MAP_SIZE; c++) {
            int idx = r * MAP_SIZE + c;
            char cell = (idx < strlen(detail->my_ships)) ? detail->my_ships[idx] : '-';
            
            SDL_Rect cell_rect = {map_start_x + c * cell_size, my_map_y + r * cell_size, cell_size - 1, cell_size - 1};
            
            // Color based on cell type
            if(cell >= '2' && cell <= '9') {
                SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255); // Ship
            } else if(cell == '@') {
                SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255); // Sunk
            } else {
                SDL_SetRenderDrawColor(renderer, 40, 60, 80, 255); // Water
            }
            SDL_RenderFillRect(renderer, &cell_rect);
        }
    }
    
    // Opponent ships
    int opp_map_x = 550;
    render_text(renderer, game->font_small, "OPPONENT SHIPS:", opp_map_x, map_y, red);
    int opp_map_y = my_map_y;
    for(int r = 0; r < MAP_SIZE; r++) {
        for(int c = 0; c < MAP_SIZE; c++) {
            int idx = r * MAP_SIZE + c;
            char cell = (idx < strlen(detail->opponent_ships)) ? detail->opponent_ships[idx] : '-';
            
            SDL_Rect cell_rect = {opp_map_x + c * cell_size, opp_map_y + r * cell_size, cell_size - 1, cell_size - 1};
            
            if(cell >= '2' && cell <= '9') {
                SDL_SetRenderDrawColor(renderer, 150, 100, 100, 255);
            } else if(cell == '@') {
                SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 40, 60, 80, 255);
            }
            SDL_RenderFillRect(renderer, &cell_rect);
        }
    }
    
    // Shot history section
    int shot_section_y = SHOT_LIST_Y;
    
    // Show shot counts
    char debug_text[128];
    snprintf(debug_text, sizeof(debug_text), "SHOT HISTORY (chronological): Total shots: %d", 
             detail->shot_count);
    render_text(renderer, game->font_small, debug_text, 50, shot_section_y - 25, white);
    
    // Use chronological shot list directly (no need to combine)
    int total_lines = detail->shot_count;
    int visible_lines = SHOT_LIST_HEIGHT / LINE_HEIGHT;
    max_scroll = (total_lines > visible_lines) ? (total_lines - visible_lines) : 0;
    if(scroll_offset > max_scroll) scroll_offset = max_scroll;
    if(scroll_offset < 0) scroll_offset = 0;
    
    // Render visible shots
    int y = shot_section_y;
    
    for(int i = scroll_offset; i < detail->shot_count && i < scroll_offset + visible_lines; i++) {
        ShotEntry* shot = &detail->all_shots[i];
        
        char shot_text[256];
        char coord[10];
        snprintf(coord, sizeof(coord), "%c%d", 'A' + shot->x, shot->y + 1);
        
        const char* player_name = shot->is_my_shot ? detail->my_name : detail->opponent_name;
        SDL_Color color = shot->is_my_shot ? cyan : red;
        
        if(shot->hit) {
            if(shot->ship_sunk) {
                snprintf(shot_text, sizeof(shot_text), "#%d %s → %s - HIT! Sunk ship (len %d)", 
                        i + 1, player_name, coord, shot->ship_length);
                render_text(renderer, game->font_small, shot_text, 50, y, yellow);
            } else {
                snprintf(shot_text, sizeof(shot_text), "#%d %s → %s - HIT ship (len %d)", 
                        i + 1, player_name, coord, shot->ship_length);
                render_text(renderer, game->font_small, shot_text, 50, y, green);
            }
        } else {
            snprintf(shot_text, sizeof(shot_text), "#%d %s → %s - MISS", 
                    i + 1, player_name, coord);
            render_text(renderer, game->font_small, shot_text, 50, y, gray);
        }
        
        y += LINE_HEIGHT;
    }
    
    // Scroll indicator
    if(max_scroll > 0) {
        char scroll_text[64];
        snprintf(scroll_text, sizeof(scroll_text), "Scroll: %d/%d (use mouse wheel)", 
                scroll_offset + 1, max_scroll + 1);
        render_text(renderer, game->font_small, scroll_text, 700, shot_section_y - 25, gray);
    }
    
    // Result at bottom
    y = 670;
    char result_text[256];
    if(detail->winner == 1) {
        snprintf(result_text, sizeof(result_text), "YOU WON!");
        render_text_centered(renderer, game->font, result_text, 500, y, green);
    } else {
        snprintf(result_text, sizeof(result_text), "YOU LOST");
        render_text_centered(renderer, game->font, result_text, 500, y, red);
    }
}

void match_detail_screen_handle_click(int mouse_x, int mouse_y, GameData* game) {
    // Back button
    if(mouse_x >= BACK_BTN_X && mouse_x <= BACK_BTN_X + BACK_BTN_W &&
       mouse_y >= BACK_BTN_Y && mouse_y <= BACK_BTN_Y + BACK_BTN_H) {
        scroll_offset = 0;  // Reset scroll
        game->state = STATE_MATCH_HISTORY;
        // game->state = STATE_LOBBY;
        return;
    }
}

void match_detail_screen_handle_wheel(int wheel_y) {
    scroll_offset -= wheel_y;  // wheel_y is positive for scroll up, negative for scroll down
    if(scroll_offset < 0) scroll_offset = 0;
    if(scroll_offset > max_scroll) scroll_offset = max_scroll;
}

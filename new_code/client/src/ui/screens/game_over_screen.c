// game_over_screen.c - Game Over screen implementation
#include "game_over_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

// ==================== RENDER GAME OVER SCREEN ====================
void game_over_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color green = {0, 255, 100, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color yellow = {255, 200, 0, 255};
    
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    int center_x = SCREEN_WIDTH / 2;
    int start_y = 100;
    
    // ==================== TITLE ====================
    SDL_Color title_color = game->game_result_won ? green : red;
    const char* title = game->game_result_won ? "VICTORY!" : "DEFEAT";
    
    // Large title with shadow effect
    SDL_Color shadow = {0, 0, 0, 255};
    render_text_centered(renderer, game->font, title, center_x + 3, start_y + 3, shadow);
    render_text_centered(renderer, game->font, title, center_x, start_y, title_color);
    
    start_y += 80;
    
    // ==================== GAME STATS ====================
    render_text_centered(renderer, game->font_small, "GAME STATISTICS", center_x, start_y, cyan);
    start_y += 40;
    
    // Stats box background (increased height for more stats)
    SDL_Rect stats_box = {200, start_y, 600, 360};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &stats_box);
    SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
    SDL_RenderDrawRect(renderer, &stats_box);
    
    int stats_y = start_y + 20;
    
    // === GAME STATISTICS ===
    char text[256];
    SDL_Color cyan_color = {100, 200, 255, 255};
    render_text_centered(renderer, game->font_small, "MATCH STATISTICS", center_x, stats_y, cyan_color);
    stats_y += 35;
    
    // Hits
    SDL_Color green_color = {100, 255, 100, 255};
    snprintf(text, sizeof(text), "Hits: %d", game->hits_count);
    render_text_centered(renderer, game->font_small, text, center_x, stats_y, green_color);
    stats_y += 30;
    
    // Misses
    SDL_Color red_color = {255, 100, 100, 255};
    snprintf(text, sizeof(text), "Misses: %d", game->misses_count);
    render_text_centered(renderer, game->font_small, text, center_x, stats_y, red_color);
    stats_y += 30;
    
    // Accuracy
    SDL_Color gold_color = {255, 215, 0, 255};
    float accuracy = game->total_shots > 0 ? 
        (float)game->hits_count / game->total_shots * 100.0f : 0.0f;
    snprintf(text, sizeof(text), "Accuracy: %.1f%%", accuracy);
    render_text_centered(renderer, game->font_small, text, center_x, stats_y, gold_color);
    stats_y += 45;
    
    // === ELO BREAKDOWN ===
    render_text_centered(renderer, game->font_small, "ELO BREAKDOWN", center_x, stats_y, cyan_color);
    stats_y += 35;
    
    // ELO Before
    snprintf(text, sizeof(text), "Previous ELO: %d", game->elo_before);
    render_text_centered(renderer, game->font_small, text, center_x, stats_y, white);
    stats_y += 30;
    
    // Base ELO Change
    SDL_Color elo_color = game->elo_change >= 0 ? green : red;
    snprintf(text, sizeof(text), "Base ELO Change: %+d", game->elo_change);
    render_text_centered(renderer, game->font_small, text, center_x, stats_y, elo_color);
    stats_y += 30;
    
    // Performance Bonus (only if won)
    if(game->game_result_won && game->elo_bonus > 0) {
        snprintf(text, sizeof(text), "Performance Bonus: +%d", game->elo_bonus);
        render_text_centered(renderer, game->font_small, text, center_x, stats_y, yellow);
        stats_y += 30;
    }
    
    // Total Change
    int total_change = game->elo_change + game->elo_bonus;
    SDL_Color total_color = total_change >= 0 ? gold : red;
    snprintf(text, sizeof(text), "Total Change: %+d", total_change);
    render_text_centered(renderer, game->font_small, text, center_x, stats_y, total_color);
    stats_y += 50;
    
    // New ELO (large and prominent)
    int new_elo = game->elo_before + total_change;
    snprintf(text, sizeof(text), "NEW ELO: %d", new_elo);
    render_text_centered(renderer, game->font, text, center_x, stats_y, gold);
    
    // ==================== BACK TO LOBBY BUTTON ====================
    int btn_x = center_x - 150;
    int btn_y = SCREEN_HEIGHT - 120;
    int btn_w = 300;
    int btn_h = 60;
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int hover = (mx >= btn_x && mx <= btn_x + btn_w &&
                 my >= btn_y && my <= btn_y + btn_h);
    
    SDL_Color btn_color = {0, 150, 255, 255};
    render_button(renderer, game->font_small, "BACK TO LOBBY", 
                 btn_x, btn_y, btn_w, btn_h, btn_color, hover, 1);
    
    // Footer message
    const char* footer = game->game_result_won ? 
        "Well played! Keep up the good work!" : 
        "Don't give up! Learn from this match!";
    render_text_centered(renderer, game->font_small, footer, center_x, SCREEN_HEIGHT - 40, cyan);
}

// ==================== HANDLE CLICK ====================
void game_over_screen_handle_click(GameData* game, int x, int y) {
    int center_x = SCREEN_WIDTH / 2;
    int btn_x = center_x - 150;
    int btn_y = SCREEN_HEIGHT - 120;
    int btn_w = 300;
    int btn_h = 60;
    
    // Back to Lobby button
    if(x >= btn_x && x <= btn_x + btn_w &&
       y >= btn_y && y <= btn_y + btn_h) {
        // Reset game state and return to lobby
        game->state = STATE_LOBBY;
        game->game_active = 0;
        
        // Reset maps
        for(int i=0; i<MAP_SIZE; i++) {
            for(int j=0; j<MAP_SIZE; j++) {
                game->own_map[i][j] = '-';
                game->enemy_map[i][j] = '-';
            }
        }
        
        // Stats and user list will be automatically refreshed by main loop
    }
}

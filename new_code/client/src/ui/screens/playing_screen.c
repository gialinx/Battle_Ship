// playing_screen.c - Game playing screen implementation
#include "playing_screen.h"
#include "../../ui/renderer.h"
#include "../../ui/colors.h"
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define OWN_MAP_X 30
#define OWN_MAP_Y 100
#define ENEMY_MAP_X 530
#define ENEMY_MAP_Y 100

// ==================== RENDER ====================
void playing_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(renderer);
    
    // Title
    render_text(renderer, game->font, "BATTLESHIP - IN BATTLE", 320, 10, COLOR_TEXT);

    // ==================== GAME TIMER ====================
    // Display elapsed time since game start
    unsigned int elapsed_ms = SDL_GetTicks() - game->game_start_time;
    int elapsed_seconds = elapsed_ms / 1000;
    int minutes = elapsed_seconds / 60;
    int seconds = elapsed_seconds % 60;
    
    char timer_text[32];
    snprintf(timer_text, sizeof(timer_text), "TIME: %d:%02d", minutes, seconds);
    render_text(renderer, game->font_small, timer_text, 850, 15, 
                (SDL_Color){255, 215, 0, 255});  // Gold color

    // Turn indicator
    if(game->is_my_turn) {
        render_text(renderer, game->font, "YOUR TURN!", 410, 40,
                   (SDL_Color){0, 255, 0, 255});
    } else {
        render_text(renderer, game->font, "Waiting...", 420, 40,
                   (SDL_Color){255, 255, 0, 255});
    }

    // Map labels
    render_text(renderer, game->font, "YOUR MAP", 160, 510, COLOR_TEXT);
    render_text(renderer, game->font, "ENEMY MAP", 660, 510, COLOR_TEXT);
    
    // Draw maps
    render_map(renderer, game->font, game->font_small, game->own_map, 
              OWN_MAP_X, OWN_MAP_Y, CELL_SIZE, 1);
    render_map(renderer, game->font, game->font_small, game->enemy_map, 
              ENEMY_MAP_X, ENEMY_MAP_Y, CELL_SIZE, 0);
    
    // Message
    if(strlen(game->message) > 0) {
        render_text(renderer, game->font, game->message, 30, 650,
                   (SDL_Color){255, 255, 0, 255});
    }

    // ==================== FORFEIT BUTTON ====================
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    int forfeit_x = 800, forfeit_y = 620;
    int forfeit_hover = (mx >= forfeit_x && mx <= forfeit_x + 150 && 
                         my >= forfeit_y && my <= forfeit_y + 45);
    
    render_button(renderer, game->font_small, "Surrender", 
                  forfeit_x, forfeit_y, 150, 45,
                  (SDL_Color){180, 0, 0, 255},  // Dark red
                  forfeit_hover, 1);
}

// ==================== HANDLE CLICK ====================
void playing_screen_handle_click(GameData* game, int x, int y) {
    // Check FORFEIT button first
    int forfeit_x = 800, forfeit_y = 620;
    if(x >= forfeit_x && x <= forfeit_x + 150 && 
       y >= forfeit_y && y <= forfeit_y + 45) {
        // Show confirmation dialog
        game->confirmation_dialog.type = 2; // DIALOG_FORFEIT_GAME
        game->confirmation_dialog.visible = 1;
        strcpy(game->confirmation_dialog.title, "SURRENDER GAME?");
        strcpy(game->confirmation_dialog.message,
               "You will LOSE this match and\n"
               "your ELO rating will decrease!\n\n"
               "Your opponent will be notified to\n"
               "approve your surrender request.");
        strcpy(game->confirmation_dialog.button1_text, "NO");
        strcpy(game->confirmation_dialog.button2_text, "YES");
        return;
    }
    
    // Only handle firing during active gameplay
    if(!game->is_my_turn) return;

    // Check if click is on enemy map
    if(x < ENEMY_MAP_X || x > ENEMY_MAP_X + MAP_SIZE * CELL_SIZE) return;
    if(y < ENEMY_MAP_Y || y > ENEMY_MAP_Y + MAP_SIZE * CELL_SIZE) return;

    int col = (x - ENEMY_MAP_X) / CELL_SIZE;
    int row = (y - ENEMY_MAP_Y) / CELL_SIZE;

    if(col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) return;

    char cell = game->enemy_map[row][col];
    if(cell == 'x' || cell == 'o' || cell == '@') {
        snprintf(game->message, sizeof(game->message), "Already fired here!");
        return;
    }

    // Will send FIRE command (handled by main client)
    snprintf(game->message, sizeof(game->message), "Firing at (%d,%d)...", col + 1, row + 1);
}


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

    // ==================== ELO BAR ====================
    // Display current ELO with prediction
    char elo_text[128];
    int display_elo = game->elo_predicted > 0 ? game->elo_predicted : game->my_elo;
    snprintf(elo_text, sizeof(elo_text), "ELO: %d", display_elo);
    render_text(renderer, game->font_small, elo_text, 850, 15, 
                (SDL_Color){255, 215, 0, 255});  // Gold color
    
    // Show predicted change if different from base
    if(game->elo_predicted > game->my_elo) {
        char pred_text[64];
        snprintf(pred_text, sizeof(pred_text), "(+%d)", game->elo_predicted - game->my_elo);
        render_text(renderer, game->font_small, pred_text, 920, 15,
                   (SDL_Color){0, 255, 100, 255});  // Green
    }

    // Turn indicator
    if(game->is_my_turn) {
        render_text(renderer, game->font, "YOUR TURN!", 410, 40,
                   (SDL_Color){0, 255, 0, 255});
    } else {
        render_text(renderer, game->font, "Waiting...", 420, 40,
                   (SDL_Color){255, 255, 0, 255});
    }

    // Map labels
    render_text(renderer, game->font, "YOUR MAP", 120, 70, COLOR_TEXT);
    render_text(renderer, game->font, "ENEMY MAP", 600, 70, COLOR_TEXT);
    
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

    // NO BACK BUTTON during gameplay - removed
}

// ==================== HANDLE CLICK ====================
void playing_screen_handle_click(GameData* game, int x, int y) {
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


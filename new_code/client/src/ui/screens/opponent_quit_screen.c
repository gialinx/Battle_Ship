// opponent_quit_screen.c - Screen when opponent quits during placement
#include "opponent_quit_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

void opponent_quit_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 200, 0, 255};
    
    // Dark background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Main dialog box
    SDL_Rect dialog = {200, 200, 600, 300};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    SDL_RenderDrawRect(renderer, &dialog);
    
    // Title
    render_text(renderer, game->font, "OPPONENT QUIT", 380, 230, yellow);
    
    // Message
    char msg[256];
    snprintf(msg, sizeof(msg), "Player %s quited game.", game->opponent_quit_name);
    render_text(renderer, game->font_small, msg, 280, 300, white);
    
    // Back to Lobby button
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    int btn_x = 400, btn_y = 400;
    int btn_hover = (mx >= btn_x && mx <= btn_x + 200 && my >= btn_y && my <= btn_y + 60);
    
    render_button(renderer, game->font_small, "Back to Lobby", 
                  btn_x, btn_y, 200, 60,
                  COLOR_BUTTON, btn_hover, 1);
}

void opponent_quit_screen_handle_click(GameData* game, int x, int y) {
    int btn_x = 400, btn_y = 400;
    
    if(x >= btn_x && x <= btn_x + 200 && y >= btn_y && y <= btn_y + 60) {
        // Reset maps
        for(int i=0; i<MAP_SIZE; i++) {
            for(int j=0; j<MAP_SIZE; j++) {
                game->own_map[i][j] = '-';
                game->enemy_map[i][j] = '-';
            }
        }
        
        game->state = STATE_LOBBY;
        game->opponent_quit_name[0] = '\0';
    }
}

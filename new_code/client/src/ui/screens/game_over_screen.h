// game_over_screen.h - Game Over screen header
#ifndef GAME_OVER_SCREEN_H
#define GAME_OVER_SCREEN_H

#include "../../core/game_data.h"
#include <SDL2/SDL.h>

// Render game over screen with ELO details
void game_over_screen_render(SDL_Renderer* renderer, GameData* game);

// Handle mouse clicks on game over screen
void game_over_screen_handle_click(GameData* game, int x, int y);

#endif

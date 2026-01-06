// surrender_request_screen.h - Screen for surrender requests
#ifndef SURRENDER_REQUEST_SCREEN_H
#define SURRENDER_REQUEST_SCREEN_H

#include "../../core/game_data.h"
#include <SDL2/SDL.h>

// Render waiting for surrender approval screen
void surrender_waiting_screen_render(SDL_Renderer* renderer, GameData* game);

// Render received surrender request screen  
void surrender_request_screen_render(SDL_Renderer* renderer, GameData* game);
void surrender_request_screen_handle_click(GameData* game, int x, int y);

#endif

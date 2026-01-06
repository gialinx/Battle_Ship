// rematch_request_screen.h - Rematch request screens
#ifndef REMATCH_REQUEST_SCREEN_H
#define REMATCH_REQUEST_SCREEN_H

#include "../../core/game_data.h"

// Waiting screen (for player who sent rematch request)
void rematch_waiting_screen_render(SDL_Renderer* renderer, GameData* game);
void rematch_waiting_screen_handle_click(GameData* game, int x, int y);

// Request screen (for player receiving rematch request)
void rematch_request_screen_render(SDL_Renderer* renderer, GameData* game);
void rematch_request_screen_handle_click(GameData* game, int x, int y);

#endif

// matchmaking_screen.h - Matchmaking screen header
#ifndef MATCHMAKING_SCREEN_H
#define MATCHMAKING_SCREEN_H

#include "../../core/game_data.h"
#include <SDL2/SDL.h>

void matchmaking_screen_render(SDL_Renderer* renderer, GameData* game);
void matchmaking_screen_handle_click(GameData* game, int x, int y);

#endif

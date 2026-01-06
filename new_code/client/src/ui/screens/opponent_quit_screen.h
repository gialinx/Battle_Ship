// opponent_quit_screen.h - Screen when opponent quits during placement
#ifndef OPPONENT_QUIT_SCREEN_H
#define OPPONENT_QUIT_SCREEN_H

#include "../../core/game_data.h"
#include <SDL2/SDL.h>

void opponent_quit_screen_render(SDL_Renderer* renderer, GameData* game);
void opponent_quit_screen_handle_click(GameData* game, int x, int y);

#endif

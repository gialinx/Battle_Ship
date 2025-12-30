// profile_screen.h - Profile screen header
#ifndef PROFILE_SCREEN_H
#define PROFILE_SCREEN_H

#include "../../core/game_data.h"

void profile_screen_render(SDL_Renderer* renderer, GameData* game);
void profile_screen_handle_click(GameData* game, int x, int y);

#endif

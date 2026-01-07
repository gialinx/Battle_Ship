// match_detail_screen.h - Detailed match replay screen
#ifndef MATCH_DETAIL_SCREEN_H
#define MATCH_DETAIL_SCREEN_H

#include "../../core/game_data.h"

void match_detail_screen_render(SDL_Renderer* renderer, GameData* game);
void match_detail_screen_handle_click(int mouse_x, int mouse_y, GameData* game);
void match_detail_screen_handle_wheel(int wheel_y);

#endif

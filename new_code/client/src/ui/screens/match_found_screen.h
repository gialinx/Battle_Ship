// match_found_screen.h - Match Found confirmation screen
#ifndef MATCH_FOUND_SCREEN_H
#define MATCH_FOUND_SCREEN_H

#include "../../core/game_data.h"

void match_found_screen_render(SDL_Renderer* renderer, TTF_Font* font, GameData* game);
void match_found_screen_handle_click(int mouse_x, int mouse_y, GameData* game);

#endif

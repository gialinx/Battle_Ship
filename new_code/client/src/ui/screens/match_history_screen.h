// match_history_screen.h - Match history list screen
#ifndef MATCH_HISTORY_SCREEN_H
#define MATCH_HISTORY_SCREEN_H

#include "../../core/game_data.h"

void match_history_screen_render(SDL_Renderer* renderer, GameData* game);
void match_history_screen_handle_click(int mouse_x, int mouse_y, GameData* game);

#endif

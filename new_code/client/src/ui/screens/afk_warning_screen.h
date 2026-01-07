#ifndef AFK_WARNING_SCREEN_H
#define AFK_WARNING_SCREEN_H

#include "../../core/game_data.h"

void afk_warning_screen_render(SDL_Renderer* renderer, GameData* game);
void afk_warning_screen_handle_event(SDL_Event* event, GameData* game);

#endif

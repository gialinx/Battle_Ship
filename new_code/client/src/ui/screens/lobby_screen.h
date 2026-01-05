// lobby_screen.h - Lobby screen
#ifndef LOBBY_SCREEN_H
#define LOBBY_SCREEN_H

#include "../../core/game_data.h"

void lobby_screen_render(SDL_Renderer* renderer, GameData* game);
void lobby_screen_handle_click(GameData* game, int x, int y);
void lobby_screen_render_invite_dialog(SDL_Renderer* renderer, GameData* game);
void lobby_screen_handle_invite_click(GameData* game, int x, int y);

#endif



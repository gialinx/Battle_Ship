// login_screen.h - Login/Register screen
#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include "../../core/game_data.h"

void login_screen_render(SDL_Renderer* renderer, GameData* game);
void login_screen_handle_click(GameData* game, int x, int y);
void login_screen_handle_text(GameData* game, const char* text);
void login_screen_handle_key(GameData* game, SDL_Keycode key);

#endif



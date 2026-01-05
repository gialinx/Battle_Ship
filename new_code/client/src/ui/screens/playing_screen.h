// playing_screen.h - Game playing screen
#ifndef PLAYING_SCREEN_H
#define PLAYING_SCREEN_H

#include "../../core/game_data.h"

// Render playing screen
void playing_screen_render(SDL_Renderer* renderer, GameData* game);

// Handle mouse click (fire)
void playing_screen_handle_click(GameData* game, int x, int y);

#endif



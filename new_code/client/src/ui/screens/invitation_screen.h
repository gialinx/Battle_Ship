// invitation_screen.h - Invitation screen header
#ifndef INVITATION_SCREEN_H
#define INVITATION_SCREEN_H

#include "../../core/game_data.h"
#include <SDL2/SDL.h>

// Render invitation screen based on state
void invitation_screen_render(SDL_Renderer* renderer, GameData* game);

// Handle mouse clicks on invitation screen
void invitation_screen_handle_click(GameData* game, int x, int y);

#endif

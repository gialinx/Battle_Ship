// placing_ships_screen.h - Ship placement screen
#ifndef PLACING_SHIPS_SCREEN_H
#define PLACING_SHIPS_SCREEN_H

#include "../../core/game_data.h"

// Initialize ship placement screen
void placing_ships_init(GameData* game);

// Render ship placement screen
void placing_ships_render(SDL_Renderer* renderer, GameData* game);

// Handle mouse click
void placing_ships_handle_click(GameData* game, int x, int y);

// Handle keyboard input
void placing_ships_handle_key(GameData* game, SDL_Keycode key);

// Handle mouse motion (for preview)
void placing_ships_handle_motion(GameData* game, int x, int y);

// Check if all ships are placed
int placing_ships_check_complete(GameData* game);

// Check if placement is valid
int placing_ships_check_valid(GameData* game, int x, int y, int length, int horizontal);

#endif



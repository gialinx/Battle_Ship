// confirmation_dialog.h - Confirmation dialog for critical actions
#ifndef CONFIRMATION_DIALOG_H
#define CONFIRMATION_DIALOG_H

#include "../../core/game_data.h"
#include <SDL2/SDL.h>

// Dialog types
typedef enum {
    DIALOG_NONE,
    DIALOG_FORFEIT_PLACEMENT,  // Leave during ship placement
    DIALOG_FORFEIT_GAME        // Forfeit during active game
} DialogType;

// Confirmation dialog structure
typedef struct {
    DialogType type;
    int visible;
    char title[100];
    char message[300];
    char button1_text[20];  // Left button (Cancel/Stay)
    char button2_text[20];  // Right button (Leave/Forfeit)
} ConfirmationDialog;

// Functions
void confirmation_dialog_render(SDL_Renderer* renderer, GameData* game);
void confirmation_dialog_handle_click(GameData* game, int x, int y);
void confirmation_dialog_show(GameData* game, DialogType type);
void confirmation_dialog_hide(GameData* game);

#endif

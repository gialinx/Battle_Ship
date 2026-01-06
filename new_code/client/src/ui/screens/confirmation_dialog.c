// confirmation_dialog.c - Confirmation dialog implementation
#include "confirmation_dialog.h"
#include "../renderer.h"
#include "../colors.h"
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#define DIALOG_WIDTH 500
#define DIALOG_HEIGHT 280
#define BUTTON_WIDTH 180
#define BUTTON_HEIGHT 50

// Show confirmation dialog
void confirmation_dialog_show(GameData* game, DialogType type) {
    game->confirmation_dialog.type = type;
    game->confirmation_dialog.visible = 1;
    
    if(type == DIALOG_FORFEIT_PLACEMENT) {
        strcpy(game->confirmation_dialog.title, "QUIT GAME?");
        strcpy(game->confirmation_dialog.message, 
               "Are you sure you want to quit?\n\n"
               "Your opponent will be notified and\n"
               "you will both return to lobby.");
        strcpy(game->confirmation_dialog.button1_text, "STAY");
        strcpy(game->confirmation_dialog.button2_text, "QUIT");
    } 
    else if(type == DIALOG_FORFEIT_GAME) {
        strcpy(game->confirmation_dialog.title, "SURRENDER GAME?");
        strcpy(game->confirmation_dialog.message,
               "You will LOSE this match and\n"
               "your ELO rating will decrease!\n\n"
               "Your opponent will be notified to\n"
               "approve your surrender request.");
        strcpy(game->confirmation_dialog.button1_text, "NO");
        strcpy(game->confirmation_dialog.button2_text, "YES");
    }
}

// Hide dialog
void confirmation_dialog_hide(GameData* game) {
    game->confirmation_dialog.visible = 0;
    game->confirmation_dialog.type = DIALOG_NONE;
}

// Render confirmation dialog
void confirmation_dialog_render(SDL_Renderer* renderer, GameData* game) {
    if(!game->confirmation_dialog.visible) return;
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, 1000, 700};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Dialog box position (centered)
    int dialog_x = (1000 - DIALOG_WIDTH) / 2;
    int dialog_y = (700 - DIALOG_HEIGHT) / 2;
    
    // Dialog background
    SDL_Rect dialog = {dialog_x, dialog_y, DIALOG_WIDTH, DIALOG_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    
    // Border
    SDL_Color border_color = (game->confirmation_dialog.type == DIALOG_FORFEIT_GAME) 
                              ? (SDL_Color){255, 50, 50, 255}   // Red for forfeit
                              : (SDL_Color){0, 200, 255, 255};  // Cyan for leave
    SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, 255);
    for(int i = 0; i < 3; i++) {
        SDL_Rect border = {dialog_x - i, dialog_y - i, DIALOG_WIDTH + i*2, DIALOG_HEIGHT + i*2};
        SDL_RenderDrawRect(renderer, &border);
    }
    
    // Warning icon and title
    SDL_Color title_color = (game->confirmation_dialog.type == DIALOG_FORFEIT_GAME)
                             ? (SDL_Color){255, 100, 100, 255}  // Light red
                             : (SDL_Color){255, 200, 0, 255};   // Yellow
    
    char full_title[120];
    if(game->confirmation_dialog.type == DIALOG_FORFEIT_GAME) {
        snprintf(full_title, sizeof(full_title), "!  %s  !", game->confirmation_dialog.title);
    } else {
        strcpy(full_title, game->confirmation_dialog.title);
    }
    
    // Calculate title position (centered)
    int title_x = dialog_x + (DIALOG_WIDTH - strlen(full_title) * 8) / 2;
    render_text(renderer, game->font_small, full_title, title_x, dialog_y + 20, title_color);
    
    // Message (multi-line)
    SDL_Color white = {255, 255, 255, 255};
    char message_copy[300];
    strcpy(message_copy, game->confirmation_dialog.message);
    
    char* line = strtok(message_copy, "\n");
    int line_y = dialog_y + 70;
    while(line != NULL) {
        int line_x = dialog_x + (DIALOG_WIDTH - strlen(line) * 6) / 2;
        render_text(renderer, game->font_small, line, line_x, line_y, white);
        line_y += 25;
        line = strtok(NULL, "\n");
    }
    
    // Buttons
    int button_y = dialog_y + DIALOG_HEIGHT - 70;
    int button1_x = dialog_x + 40;
    int button2_x = dialog_x + DIALOG_WIDTH - BUTTON_WIDTH - 40;
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    // Button 1 (Cancel/Stay) - Green
    int button1_hover = (mx >= button1_x && mx <= button1_x + BUTTON_WIDTH &&
                         my >= button_y && my <= button_y + BUTTON_HEIGHT);
    render_button(renderer, game->font_small, game->confirmation_dialog.button1_text,
                  button1_x, button_y, BUTTON_WIDTH, BUTTON_HEIGHT,
                  (SDL_Color){0, 180, 80, 255}, button1_hover, 1);
    
    // Button 2 (Leave/Forfeit) - Red
    int button2_hover = (mx >= button2_x && mx <= button2_x + BUTTON_WIDTH &&
                         my >= button_y && my <= button_y + BUTTON_HEIGHT);
    render_button(renderer, game->font_small, game->confirmation_dialog.button2_text,
                  button2_x, button_y, BUTTON_WIDTH, BUTTON_HEIGHT,
                  (SDL_Color){200, 40, 40, 255}, button2_hover, 1);
}

// Handle click on dialog
void confirmation_dialog_handle_click(GameData* game, int x, int y) {
    if(!game->confirmation_dialog.visible) return;
    
    int dialog_x = (1000 - DIALOG_WIDTH) / 2;
    int dialog_y = (700 - DIALOG_HEIGHT) / 2;
    int button_y = dialog_y + DIALOG_HEIGHT - 70;
    int button1_x = dialog_x + 40;
    int button2_x = dialog_x + DIALOG_WIDTH - BUTTON_WIDTH - 40;
    
    // Button 1 (Cancel/Stay)
    if(x >= button1_x && x <= button1_x + BUTTON_WIDTH &&
       y >= button_y && y <= button_y + BUTTON_HEIGHT) {
        // Just close dialog, do nothing
        confirmation_dialog_hide(game);
        printf("Dialog cancelled\n");
        return;
    }
    
    // Button 2 (Leave/Forfeit)
    if(x >= button2_x && x <= button2_x + BUTTON_WIDTH &&
       y >= button_y && y <= button_y + BUTTON_HEIGHT) {
        
        DialogType type = game->confirmation_dialog.type;
        confirmation_dialog_hide(game);
        
        if(type == DIALOG_FORFEIT_PLACEMENT) {
            // Send FORFEIT command
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "FORFEIT#");
            send(game->sockfd, buffer, strlen(buffer), 0);
            printf("CLIENT: Sent FORFEIT (during placement)\n");
            
            // Reset local state
            game->state = STATE_LOBBY;
            for(int i=0; i<MAP_SIZE; i++)
                for(int j=0; j<MAP_SIZE; j++)
                    game->own_map[i][j] = '-';
            
            strcpy(game->message, "You left the match");
        }
        else if(type == DIALOG_FORFEIT_GAME) {
            // Send SURRENDER_REQUEST command
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "SURRENDER_REQUEST#");
            send(game->sockfd, buffer, strlen(buffer), 0);
            printf("CLIENT: Sent SURRENDER_REQUEST\n");
            
            // Go to waiting screen
            game->state = STATE_WAITING_SURRENDER_APPROVAL;
            strcpy(game->message, "Waiting for opponent approval...");
        }
        
        return;
    }
    
    // Click outside dialog - do nothing (force button click)
}

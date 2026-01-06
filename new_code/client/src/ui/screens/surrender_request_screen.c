// surrender_request_screen.c - Surrender request screens
#include "surrender_request_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

// Render waiting screen (for player who requested surrender)
void surrender_waiting_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    
    // Dark background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Main dialog box
    SDL_Rect dialog = {200, 200, 600, 300};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog);
    
    // Title
    render_text(renderer, game->font, "WAITING FOR APPROVAL", 300, 230, cyan);
    
    // Spinner
    unsigned int ticks = SDL_GetTicks();
    int spinner_frame = (ticks / 200) % 8;
    const char* spinner_chars[] = {"|", "/", "-", "\\", "|", "/", "-", "\\"};
    render_text(renderer, game->font, spinner_chars[spinner_frame], 480, 290, cyan);
    
    // Message
    render_text(renderer, game->font_small, "Waiting for opponent to respond...", 310, 340, white);
}

// Render surrender request dialog (for player receiving request)
void surrender_request_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Render dark background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 200, 0, 255};
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Dialog box
    SDL_Rect dialog = {150, 150, 700, 400};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    
    // Yellow border
    SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    for(int i = 0; i < 3; i++) {
        SDL_Rect border = {dialog.x - i, dialog.y - i, dialog.w + i*2, dialog.h + i*2};
        SDL_RenderDrawRect(renderer, &border);
    }
    
    // Title
    render_text(renderer, game->font, "SURRENDER REQUEST", 330, 180, yellow);
    
    // Message
    char msg[256];
    snprintf(msg, sizeof(msg), "Player %s wants to surrender!", game->surrender_requester_name);
    render_text(renderer, game->font_small, msg, 250, 250, white);
    
    render_text(renderer, game->font_small, "Do you accept their surrender?", 290, 290, white);
    render_text(renderer, game->font_small, "Accept = You WIN, Decline = Continue game", 230, 330, 
                (SDL_Color){180, 180, 180, 255});
    
    // Buttons
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    int decline_x = 230, accept_x = 520, btn_y = 420;
    int btn_w = 200, btn_h = 60;
    
    int decline_hover = (mx >= decline_x && mx <= decline_x + btn_w && 
                         my >= btn_y && my <= btn_y + btn_h);
    int accept_hover = (mx >= accept_x && mx <= accept_x + btn_w &&
                        my >= btn_y && my <= btn_y + btn_h);
    
    // Decline button (left)
    render_button(renderer, game->font_small, "Decline", 
                  decline_x, btn_y, btn_w, btn_h,
                  (SDL_Color){200, 100, 0, 255}, decline_hover, 1);
    
    // Accept button (right)
    render_button(renderer, game->font_small, "Accept", 
                  accept_x, btn_y, btn_w, btn_h,
                  (SDL_Color){0, 180, 80, 255}, accept_hover, 1);
}

void surrender_request_screen_handle_click(GameData* game, int x, int y) {
    int decline_x = 230, accept_x = 520, btn_y = 420;
    int btn_w = 200, btn_h = 60;
    
    printf("Surrender screen click: x=%d, y=%d\n", x, y);
    
    // Decline button
    if(x >= decline_x && x <= decline_x + btn_w && 
       y >= btn_y && y <= btn_y + btn_h) {
        // Send SURRENDER_DECLINE
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "SURRENDER_DECLINE#");
        send(game->sockfd, buffer, strlen(buffer), 0);
        printf("CLIENT: Declined surrender request\n");
        
        // Back to playing
        game->state = STATE_PLAYING;
        strcpy(game->message, "You declined the surrender request");
        game->surrender_requester_name[0] = '\0';
    }
    // Accept button
    else if(x >= accept_x && x <= accept_x + btn_w &&
            y >= btn_y && y <= btn_y + btn_h) {
        // Send SURRENDER_ACCEPT
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "SURRENDER_ACCEPT#");
        send(game->sockfd, buffer, strlen(buffer), 0);
        printf("CLIENT: Accepted surrender request\n");
        
        // Will receive GAME_OVER from server
        strcpy(game->message, "Surrender accepted, you win!");
    }
}

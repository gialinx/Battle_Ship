#include "afk_warning_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include "../../network/network.h"
#include <string.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

void afk_warning_screen_render(SDL_Renderer* renderer, GameData* game) {
    if(!game->afk_warning_visible) return;
    
    // Calculate remaining time (2 minutes = 120 seconds)
    unsigned int elapsed_ms = SDL_GetTicks() - game->afk_warning_time;
    int elapsed_seconds = elapsed_ms / 1000;
    int remaining_seconds = 120 - elapsed_seconds;
    if(remaining_seconds < 0) remaining_seconds = 0;
    
    int minutes = remaining_seconds / 60;
    int seconds = remaining_seconds % 60;
    
    // Blinking effect for urgency (use SDL ticks to create pulse effect)
    unsigned int ticks = SDL_GetTicks();
    int pulse = (ticks / 500) % 2;  // Blink every 500ms
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Dialog box - increased height to prevent button overlap
    int box_w = 500;
    int box_h = 300;  // Increased from 250 to 300
    int box_x = (SCREEN_WIDTH - box_w) / 2;
    int box_y = (SCREEN_HEIGHT - box_h) / 2;
    
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_Rect box = {box_x, box_y, box_w, box_h};
    SDL_RenderFillRect(renderer, &box);
    
    // Blinking orange/red border for attention
    if(pulse) {
        SDL_SetRenderDrawColor(renderer, 255, 150, 0, 255);  // Orange
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);   // Red
    }
    SDL_RenderDrawRect(renderer, &box);
    
    // Draw thicker border for more attention
    SDL_Rect box2 = {box_x - 1, box_y - 1, box_w + 2, box_h + 2};
    SDL_RenderDrawRect(renderer, &box2);
    SDL_Rect box3 = {box_x - 2, box_y - 2, box_w + 4, box_h + 4};
    SDL_RenderDrawRect(renderer, &box3);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color orange = {255, 150, 0, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    
    // Title with blinking color
    render_text_centered(renderer, game->font, "⚠ AFK WARNING ⚠", 
                        SCREEN_WIDTH / 2, box_y + 30, pulse ? orange : red);
    
    // Message
    render_text_centered(renderer, game->font_small, "Are you still there?", 
                        SCREEN_WIDTH / 2, box_y + 80, white);
    render_text_centered(renderer, game->font_small, "You have been inactive for 1 minute.", 
                        SCREEN_WIDTH / 2, box_y + 110, white);
    
    // Countdown timer
    char countdown_text[64];
    snprintf(countdown_text, sizeof(countdown_text), 
             "Time remaining: %d:%02d", minutes, seconds);
    render_text_centered(renderer, game->font_small, countdown_text,
                        SCREEN_WIDTH / 2, box_y + 140, yellow);
    
    render_text_centered(renderer, game->font_small, "If you don't respond,", 
                        SCREEN_WIDTH / 2, box_y + 170, white);
    render_text_centered(renderer, game->font_small, "you will forfeit the match.", 
                        SCREEN_WIDTH / 2, box_y + 195, white);
    
    // OK button - moved down to avoid overlap
    int btn_w = 150;
    int btn_h = 50;
    int btn_x = (SCREEN_WIDTH - btn_w) / 2;
    int btn_y = box_y + box_h - 70;  // Changed from -80 to -70
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int btn_hover = (mx >= btn_x && mx <= btn_x + btn_w && 
                     my >= btn_y && my <= btn_y + btn_h);
    
    SDL_Color green = {50, 200, 50, 255};
    render_button(renderer, game->font, "YES", btn_x, btn_y, btn_w, btn_h, 
                 green, btn_hover, 1);
}

void afk_warning_screen_handle_event(SDL_Event* event, GameData* game) {
    if(!game->afk_warning_visible) return;
    
    if(event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        int mx = event->button.x;
        int my = event->button.y;
        
        int btn_w = 150;
        int btn_h = 50;
        int btn_x = (SCREEN_WIDTH - btn_w) / 2;
        
        int box_h = 300;  // Updated to match new box height
        int box_y = (SCREEN_HEIGHT - box_h) / 2;
        int btn_y = box_y + box_h - 70;  // Updated to match new button position
        
        // OK button
        if(mx >= btn_x && mx <= btn_x + btn_w && 
           my >= btn_y && my <= btn_y + btn_h) {
            // Send AFK response to server
            send_to_server(game->sockfd, "AFK_RESPONSE#");
            game->afk_warning_visible = 0;
            printf("CLIENT: Responded to AFK warning\n");
        }
    }
}

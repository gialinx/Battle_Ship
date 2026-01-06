#include "afk_warning_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include "../../network/network.h"
#include <string.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

void afk_warning_screen_render(SDL_Renderer* renderer, GameData* game) {
    if(!game->afk_warning_visible) return;
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Dialog box
    int box_w = 500;
    int box_h = 250;
    int box_x = (SCREEN_WIDTH - box_w) / 2;
    int box_y = (SCREEN_HEIGHT - box_h) / 2;
    
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_Rect box = {box_x, box_y, box_w, box_h};
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 255, 150, 0, 255);  // Orange border
    SDL_RenderDrawRect(renderer, &box);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color orange = {255, 150, 0, 255};
    
    // Title
    render_text_centered(renderer, game->font, "⚠ AFK WARNING ⚠", 
                        SCREEN_WIDTH / 2, box_y + 30, orange);
    
    // Message
    render_text_centered(renderer, game->font_small, "Are you still there?", 
                        SCREEN_WIDTH / 2, box_y + 80, white);
    render_text_centered(renderer, game->font_small, "You have been inactive for 3 minutes.", 
                        SCREEN_WIDTH / 2, box_y + 110, white);
    render_text_centered(renderer, game->font_small, "If you don't respond in 2 minutes,", 
                        SCREEN_WIDTH / 2, box_y + 140, white);
    render_text_centered(renderer, game->font_small, "you will forfeit the match.", 
                        SCREEN_WIDTH / 2, box_y + 165, white);
    
    // OK button
    int btn_w = 150;
    int btn_h = 50;
    int btn_x = (SCREEN_WIDTH - btn_w) / 2;
    int btn_y = box_y + box_h - 80;
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int btn_hover = (mx >= btn_x && mx <= btn_x + btn_w && 
                     my >= btn_y && my <= btn_y + btn_h);
    
    SDL_Color green = {50, 200, 50, 255};
    render_button(renderer, game->font_small, "I'M HERE!", btn_x, btn_y, btn_w, btn_h, 
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
        
        int box_h = 250;
        int box_y = (SCREEN_HEIGHT - box_h) / 2;
        int btn_y = box_y + box_h - 80;
        
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

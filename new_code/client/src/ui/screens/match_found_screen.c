// match_found_screen.c - Clean Match Found screen with Accept/Decline
#include "match_found_screen.h"
#include "../colors.h"
#include "../renderer.h"
#include <stdio.h>
#include <string.h>

// External function to send message to server (defined in client_gui_complete.c)
extern void send_msg(const char* msg);

#define CARD_X 250
#define CARD_Y 150
#define CARD_W 500
#define CARD_H 400

#define ACCEPT_BTN_X 300
#define ACCEPT_BTN_Y 480
#define ACCEPT_BTN_W 180
#define ACCEPT_BTN_H 60

#define DECLINE_BTN_X 520
#define DECLINE_BTN_Y 480
#define DECLINE_BTN_W 180
#define DECLINE_BTN_H 60

void match_found_screen_render(SDL_Renderer* renderer, TTF_Font* font, GameData* game) {
    // Dark overlay background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
    SDL_Rect overlay = {0, 0, 1000, 700};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Main card with gradient border effect
    SDL_Rect card_border = {CARD_X - 3, CARD_Y - 3, CARD_W + 6, CARD_H + 6};
    SDL_SetRenderDrawColor(renderer, 0, 150, 200, 255);
    SDL_RenderFillRect(renderer, &card_border);
    
    SDL_Rect card = {CARD_X, CARD_Y, CARD_W, CARD_H};
    SDL_SetRenderDrawColor(renderer, 30, 40, 55, 255);
    SDL_RenderFillRect(renderer, &card);
    
    // Colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 220, 255, 255};
    SDL_Color gold = {255, 200, 50, 255};
    SDL_Color green = {50, 255, 150, 255};
    
    // Title "MATCH FOUND!"
    render_text_centered(renderer, font, "MATCH FOUND!", 500, 200, cyan);
    
    // Decorative line
    SDL_SetRenderDrawColor(renderer, 0, 150, 200, 255);
    SDL_Rect line1 = {350, 230, 300, 2};
    SDL_RenderFillRect(renderer, &line1);
    
    // Opponent info box
    SDL_Rect info_box = {320, 260, 360, 140};
    SDL_SetRenderDrawColor(renderer, 20, 30, 45, 255);
    SDL_RenderFillRect(renderer, &info_box);
    
    SDL_SetRenderDrawColor(renderer, 0, 180, 220, 180);
    SDL_RenderDrawRect(renderer, &info_box);
    
    // "VS" label
    render_text_centered(renderer, font, "VS", 500, 280, white);
    
    // Opponent username (larger, centered)
    render_text_centered(renderer, font, game->matched_opponent_name, 500, 320, cyan);
    
    // ID and ELO on same line
    char id_text[128];
    snprintf(id_text, sizeof(id_text), "ID: %d", game->matched_opponent_id);
    render_text_centered(renderer, font, id_text, 410, 360, gold);
    
    char elo_text[128];
    snprintf(elo_text, sizeof(elo_text), "ELO: %d", game->matched_opponent_elo);
    render_text_centered(renderer, font, elo_text, 590, 360, green);
    
    // Get mouse position for hover effects
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    
    // ACCEPT button
    SDL_Rect accept_btn = {ACCEPT_BTN_X, ACCEPT_BTN_Y, ACCEPT_BTN_W, ACCEPT_BTN_H};
    int accept_hover = (mouse_x >= ACCEPT_BTN_X && mouse_x <= ACCEPT_BTN_X + ACCEPT_BTN_W &&
                        mouse_y >= ACCEPT_BTN_Y && mouse_y <= ACCEPT_BTN_Y + ACCEPT_BTN_H);
    
    if (accept_hover) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 120, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 200, 100, 255);
    }
    SDL_RenderFillRect(renderer, &accept_btn);
    
    SDL_SetRenderDrawColor(renderer, 0, 255, 150, 255);
    SDL_RenderDrawRect(renderer, &accept_btn);
    
    render_text_centered(renderer, font, "ACCEPT!", ACCEPT_BTN_X + ACCEPT_BTN_W/2, 
                        ACCEPT_BTN_Y + ACCEPT_BTN_H/2 - 10, white);
    
    // DECLINE button
    SDL_Rect decline_btn = {DECLINE_BTN_X, DECLINE_BTN_Y, DECLINE_BTN_W, DECLINE_BTN_H};
    int decline_hover = (mouse_x >= DECLINE_BTN_X && mouse_x <= DECLINE_BTN_X + DECLINE_BTN_W &&
                         mouse_y >= DECLINE_BTN_Y && mouse_y <= DECLINE_BTN_Y + DECLINE_BTN_H);
    
    if (decline_hover) {
        SDL_SetRenderDrawColor(renderer, 200, 60, 60, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 150, 50, 50, 255);
    }
    SDL_RenderFillRect(renderer, &decline_btn);
    
    SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &decline_btn);
    
    render_text_centered(renderer, font, "DECLINE", DECLINE_BTN_X + DECLINE_BTN_W/2, 
                        DECLINE_BTN_Y + DECLINE_BTN_H/2 - 10, white);
    
    // Countdown timer
    int elapsed = (SDL_GetTicks() - game->matchmaking_start_time) / 1000;
    int remaining = 30 - elapsed;
    if (remaining < 0) remaining = 0;
    
    char timer_text[64];
    snprintf(timer_text, sizeof(timer_text), "Time remaining: %d seconds", remaining);
    
    SDL_Color timer_color = remaining <= 10 ? 
        (SDL_Color){255, 100, 100, 255} : 
        (SDL_Color){180, 180, 180, 255};
    
    render_text_centered(renderer, font, timer_text, 500, 570, timer_color);
}

void match_found_screen_handle_click(int mouse_x, int mouse_y, GameData* game) {
    // Check ACCEPT button
    if (mouse_x >= ACCEPT_BTN_X && mouse_x <= ACCEPT_BTN_X + ACCEPT_BTN_W &&
        mouse_y >= ACCEPT_BTN_Y && mouse_y <= ACCEPT_BTN_Y + ACCEPT_BTN_H) {
        
        // Send ACCEPT_MATCH to server
        send_msg("ACCEPT_MATCH");
        
        printf("Accepted match with %s\n", game->matched_opponent_name);
        
        // Transition will happen when server confirms
        return;
    }
    
    // Check DECLINE button
    if (mouse_x >= DECLINE_BTN_X && mouse_x <= DECLINE_BTN_X + DECLINE_BTN_W &&
        mouse_y >= DECLINE_BTN_Y && mouse_y <= DECLINE_BTN_Y + DECLINE_BTN_H) {
        
        // Send DECLINE_MATCH to server
        send_msg("DECLINE_MATCH");
        
        printf("Declined match with %s\n", game->matched_opponent_name);
        
        // Go back to lobby
        game->state = STATE_LOBBY;
        game->matchmaking_active = 0;
        return;
    }
}

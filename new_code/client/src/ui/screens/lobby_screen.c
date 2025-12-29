// lobby_screen.c - Lobby screen implementation
#include "lobby_screen.h"
#include "../../ui/renderer.h"
#include "../../ui/colors.h"
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

void lobby_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color gray = {150, 150, 150, 255};
    
    // Header
    char header[256];
    snprintf(header, sizeof(header), "Welcome %s! (ELO: %d, Games: %d, Wins: %d)", 
             game->my_username, game->my_elo, game->total_games, game->wins);
    render_text(renderer, game->font_small, header, 50, 20, cyan);
    
    render_text(renderer, game->font, "ONLINE PLAYERS LIST", 350, 60, white);
    
    // User list
    int y = 120;
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    for(int i = game->scroll_offset; i < game->user_count && i < game->scroll_offset + 10; i++) {
        UserInfo* u = &game->users[i];
        
        if(u->user_id == game->my_user_id) continue;
        
        SDL_Rect user_rect = {50, y, 900, 45};
        
        // Background
        if(i == game->selected_user_index) {
            SDL_SetRenderDrawColor(renderer, 0, 80, 120, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
        }
        SDL_RenderFillRect(renderer, &user_rect);
        
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &user_rect);
        
        // Username
        char user_text[256];
        snprintf(user_text, sizeof(user_text), "%s (ELO: %d)", u->username, u->elo_rating);
        render_text(renderer, game->font_small, user_text, 60, y + 10, white);
        
        // Status
        SDL_Color status_color = strcmp(u->status, "online") == 0 ? green : gray;
        render_text(renderer, game->font_small, u->status, 700, y + 10, status_color);
        
        // Invite button (only for online users)
        if(strcmp(u->status, "online") == 0) {
            int invite_hover = (mx >= 800 && mx <= 920 && my >= y + 5 && my <= y + 40);
            render_button(renderer, game->font_small, "Invite to play", 800, y + 5, 120, 35, 
                         (SDL_Color){0, 150, 0, 255}, invite_hover, 1);
        }
        
        y += 50;
    }
    
    // Buttons
    int refresh_hover = (mx >= 50 && mx <= 200 && my >= 620 && my <= 670);
    int logout_hover = (mx >= 220 && mx <= 370 && my >= 620 && my <= 670);
    
    render_button(renderer, game->font_small, "Refresh", 50, 620, 150, 50, 
                 (SDL_Color){0, 100, 200, 255}, refresh_hover, 1);
    render_button(renderer, game->font_small, "Logout", 220, 620, 150, 50, 
                 (SDL_Color){200, 0, 0, 255}, logout_hover, 1);
}

void lobby_screen_handle_click(GameData* game, int x, int y) {
    // Refresh button
    if(x >= 50 && x <= 200 && y >= 620 && y <= 670) {
        // Will be handled by main client
        return;
    }
    
    // Logout button
    if(x >= 220 && x <= 370 && y >= 620 && y <= 670) {
        // Will be handled by main client
        game->state = STATE_LOGIN;
        memset(&game->username_field.text, 0, sizeof(game->username_field.text));
        memset(&game->password_field.text, 0, sizeof(game->password_field.text));
        strcpy(game->login_message, "Logged out");
        return;
    }
    
    // User list clicks
    int list_y = 120;
    for(int i = game->scroll_offset; i < game->user_count && i < game->scroll_offset + 10; i++) {
        UserInfo* u = &game->users[i];
        
        if(u->user_id == game->my_user_id) continue;
        
        // Select user
        if(x >= 50 && x <= 950 && y >= list_y && y <= list_y + 45) {
            game->selected_user_index = i;
            
            // Invite button
            if(x >= 800 && x <= 920 && strcmp(u->status, "online") == 0) {
                game->invited_user_id = u->user_id;
                strcpy(game->invited_username, u->username);
                game->state = STATE_WAITING_INVITE;
                // Will be handled by main client to send INVITE command
            }
            return;
        }
        list_y += 50;
    }
}

void lobby_screen_render_invite_dialog(SDL_Renderer* renderer, GameData* game) {
    // Darken background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);

    // Dialog box
    SDL_Rect dialog = {250, 200, 500, 300};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // X button (close button) at top-right corner
    int close_x = 720, close_y = 205;
    int close_size = 25;
    int close_hover = (mx >= close_x && mx <= close_x + close_size &&
                       my >= close_y && my <= close_y + close_size);

    SDL_Rect close_btn = {close_x, close_y, close_size, close_size};
    SDL_SetRenderDrawColor(renderer, close_hover ? 255 : 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &close_btn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &close_btn);
    render_text(renderer, game->font_small, "X", close_x + 7, close_y + 2, white);

    if(game->state == STATE_WAITING_INVITE) {
        render_text(renderer, game->font, "Waiting for response...", 320, 250, cyan);
        char msg[256];
        snprintf(msg, sizeof(msg), "Invitation sent to %s", game->invited_username);
        render_text(renderer, game->font_small, msg, 300, 300, white);

        int cancel_hover = (mx >= 400 && mx <= 600 && my >= 400 && my <= 450);
        render_button(renderer, game->font_small, "Cancel Invite", 400, 400, 200, 50,
                     (SDL_Color){200, 0, 0, 255}, cancel_hover, 1);
    }
    else if(game->state == STATE_RECEIVED_INVITE) {
        render_text(renderer, game->font, "Game invitation!", 340, 250, cyan);
        char msg[256];
        snprintf(msg, sizeof(msg), "%s wants to play a game with you", game->inviter_username);
        render_text(renderer, game->font_small, msg, 270, 300, white);

        int accept_hover = (mx >= 280 && mx <= 460 && my >= 400 && my <= 450);
        int decline_hover = (mx >= 480 && mx <= 660 && my >= 400 && my <= 450);

        render_button(renderer, game->font_small, "Accept", 280, 400, 180, 50,
                     (SDL_Color){0, 150, 0, 255}, accept_hover, 1);
        render_button(renderer, game->font_small, "Decline", 480, 400, 180, 50,
                     (SDL_Color){200, 0, 0, 255}, decline_hover, 1);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void lobby_screen_handle_invite_click(GameData* game, int x, int y) {
    // X button (close) - works for both states
    int close_x = 720, close_y = 205, close_size = 25;
    if(x >= close_x && x <= close_x + close_size && y >= close_y && y <= close_y + close_size) {
        if(game->state == STATE_WAITING_INVITE) {
            game->state = STATE_LOBBY;
            // Will be handled by main client to send CANCEL_INVITE
        } else if(game->state == STATE_RECEIVED_INVITE) {
            game->state = STATE_LOBBY;
            // Will be handled by main client to send DECLINE_INVITE
        }
        return;
    }

    if(game->state == STATE_WAITING_INVITE) {
        // Cancel button
        if(x >= 400 && x <= 600 && y >= 400 && y <= 450) {
            game->state = STATE_LOBBY;
            // Will be handled by main client to send CANCEL_INVITE
        }
    }
    else if(game->state == STATE_RECEIVED_INVITE) {
        // Accept button
        if(x >= 280 && x <= 460 && y >= 400 && y <= 450) {
            // Will be handled by main client to send ACCEPT_INVITE
        }
        // Decline button
        else if(x >= 480 && x <= 660 && y >= 400 && y <= 450) {
            game->state = STATE_LOBBY;
            // Will be handled by main client to send DECLINE_INVITE
        }
    }
}


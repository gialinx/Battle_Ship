// invitation_screen.c - Invitation screen implementation
#include "invitation_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

// ==================== RENDER INVITATION SCREEN ====================
void invitation_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color green = {0, 200, 0, 255};
    SDL_Color red = {200, 0, 0, 255};
    SDL_Color yellow = {255, 200, 0, 255};

    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);

    // Dialog box
    SDL_Rect dialog = {200, 150, 600, 400};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
    for(int i = 0; i < 3; i++) {
        SDL_Rect border = {dialog.x - i, dialog.y - i, dialog.w + 2*i, dialog.h + 2*i};
        SDL_RenderDrawRect(renderer, &border);
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // ==================== STATE: SENDING INVITE ====================
    if(game->state == STATE_SENDING_INVITE) {
        // Title
        render_text(renderer, game->font, "Sending Invitation...", 330, 200, cyan);

        // Message
        char msg[256];
        snprintf(msg, sizeof(msg), "Sending invite to %s...", game->invited_username);
        render_text(renderer, game->font_small, msg, 280, 270, white);

        // Spinner animation
        unsigned int ticks = SDL_GetTicks();
        int spinner_frame = (ticks / 200) % 4;
        const char* spinner_chars[] = {"|", "/", "-", "\\"};
        char spinner[64];
        snprintf(spinner, sizeof(spinner), "Please wait %s", spinner_chars[spinner_frame]);
        render_text(renderer, game->font_small, spinner, 380, 320, yellow);
    }
    // ==================== STATE: WAITING FOR RESPONSE ====================
    else if(game->state == STATE_WAITING_INVITE) {
        // Title
        render_text(renderer, game->font, "Invitation Sent", 390, 200, green);

        // Message
        char msg[256];
        snprintf(msg, sizeof(msg), "Invitation sent to %s.", game->invited_username);
        render_text(renderer, game->font_small, msg, 320, 300, white);
        render_text(renderer, game->font_small, "Waiting for response...", 380, 340, yellow);

        // Cancel button
        int cancel_x = 350;
        int cancel_y = 420;
        int cancel_w = 300;
        int cancel_h = 60;
        int cancel_hover = (mx >= cancel_x && mx <= cancel_x + cancel_w &&
                           my >= cancel_y && my <= cancel_y + cancel_h);

        render_button(renderer, game->font, "CANCEL", cancel_x, cancel_y, cancel_w, cancel_h,
                     red, cancel_hover, 1);
    }
    // ==================== STATE: RECEIVED INVITE ====================
    else if(game->state == STATE_RECEIVED_INVITE) {
        // Title
        render_text(renderer, game->font, "Game Invitation", 390, 200, cyan);

        // Message
        char msg[256];
        snprintf(msg, sizeof(msg), "%s has sent you an invitation to play", 
                 game->inviter_username);
        render_text(renderer, game->font_small, msg, 280, 270, white);

        // Accept button
        int accept_x = 250;
        int accept_y = 380;
        int accept_w = 200;
        int accept_h = 60;
        int accept_hover = (mx >= accept_x && mx <= accept_x + accept_w &&
                           my >= accept_y && my <= accept_y + accept_h);

        render_button(renderer, game->font, "ACCEPT", accept_x, accept_y, accept_w, accept_h,
                     green, accept_hover, 1);

        // Decline button
        int decline_x = 550;
        int decline_y = 380;
        int decline_w = 200;
        int decline_h = 60;
        int decline_hover = (mx >= decline_x && mx <= decline_x + decline_w &&
                            my >= decline_y && my <= decline_y + decline_h);

        render_button(renderer, game->font, "DECLINE", decline_x, decline_y, decline_w, decline_h,
                     red, decline_hover, 1);
    }
    // ==================== STATE: INVITE DECLINED (người mời nhận thông báo) ====================
    else if(game->state == STATE_LOBBY && strlen(game->message) > 0 && 
            strstr(game->message, "declined") != NULL) {
        // Title
        render_text(renderer, game->font, "Invitation Declined", 320, 200, red);

        // Message - show who declined
        render_text(renderer, game->font_small, game->message, 250, 280, white);

        // Close button (X)
        int close_x = 450;
        int close_y = 400;
        int close_size = 60;
        int close_hover = (mx >= close_x && mx <= close_x + close_size &&
                          my >= close_y && my <= close_y + close_size);

        SDL_Rect close_btn = {close_x, close_y, close_size, close_size};
        SDL_Color close_color = close_hover ? (SDL_Color){255, 100, 100, 255} : red;
        SDL_SetRenderDrawColor(renderer, close_color.r, close_color.g, close_color.b, 255);
        SDL_RenderFillRect(renderer, &close_btn);
        SDL_SetRenderDrawColor(renderer, white.r, white.g, white.b, 255);
        SDL_RenderDrawRect(renderer, &close_btn);

        render_text(renderer, game->font, "X", close_x + 18, close_y + 10, white);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// ==================== HANDLE CLICKS ON INVITATION SCREEN ====================
void invitation_screen_handle_click(GameData* game, int x, int y) {
    // STATE_WAITING_INVITE: Cancel button
    if(game->state == STATE_WAITING_INVITE) {
        int cancel_x = 350;
        int cancel_y = 420;
        int cancel_w = 300;
        int cancel_h = 60;

        if(x >= cancel_x && x <= cancel_x + cancel_w &&
           y >= cancel_y && y <= cancel_y + cancel_h) {
            // Send cancel request to server
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "CANCEL_INVITE#");
            send(game->sockfd, buffer, strlen(buffer), 0);
            printf("CLIENT: Cancelled invitation\n");
            
            game->state = STATE_LOBBY;
            strcpy(game->message, "Invitation cancelled");
        }
    }
    // STATE_RECEIVED_INVITE: Accept or Decline
    else if(game->state == STATE_RECEIVED_INVITE) {
        int accept_x = 250;
        int accept_y = 380;
        int accept_w = 200;
        int accept_h = 60;

        int decline_x = 550;
        int decline_y = 380;
        int decline_w = 200;
        int decline_h = 60;

        // Accept button clicked
        if(x >= accept_x && x <= accept_x + accept_w &&
           y >= accept_y && y <= accept_y + accept_h) {
            // Send accept to server
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "ACCEPT_INVITE:%d#", game->inviter_user_id);
            send(game->sockfd, buffer, strlen(buffer), 0);
            printf("CLIENT: Accepted invitation from user_id=%d\n", game->inviter_user_id);
        }
        // Decline button clicked
        else if(x >= decline_x && x <= decline_x + decline_w &&
                y >= decline_y && y <= decline_y + decline_h) {
            // Send decline to server
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "DECLINE_INVITE:%d#", game->inviter_user_id);
            send(game->sockfd, buffer, strlen(buffer), 0);
            printf("CLIENT: Declined invitation from user_id=%d\n", game->inviter_user_id);
            
            game->state = STATE_LOBBY;
            strcpy(game->message, "Invitation declined");
        }
    }
    // Handle close button on decline notification
    else if(game->state == STATE_LOBBY && strlen(game->message) > 0 && 
            strstr(game->message, "declined") != NULL) {
        int close_x = 450;
        int close_y = 400;
        int close_size = 60;

        if(x >= close_x && x <= close_x + close_size &&
           y >= close_y && y <= close_y + close_size) {
            // Clear message and return to lobby
            game->message[0] = '\0';
        }
    }
}

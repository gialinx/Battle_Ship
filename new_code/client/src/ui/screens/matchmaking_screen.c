// matchmaking_screen.c - Matchmaking screen (finding match)
#include "matchmaking_screen.h"
#include "../renderer.h"
#include "../colors.h"
#include <stdio.h>
#include <sys/socket.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

void matchmaking_screen_render(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color gray = {150, 150, 150, 255};
    SDL_Color red = {200, 0, 0, 255};

    // Dark background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);

    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);

    // Main dialog box
    SDL_Rect dialog = {200, 150, 600, 400};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog);

    // Title
    render_text(renderer, game->font, "FINDING MATCH", 350, 180, cyan);

    // Animated spinner
    unsigned int ticks = SDL_GetTicks();
    int spinner_frame = (ticks / 200) % 8;
    const char* spinner_chars[] = {"|", "/", "-", "\\", "|", "/", "-", "\\"};
    render_text(renderer, game->font, spinner_chars[spinner_frame], 480, 250, cyan);

    // Status message
    render_text(renderer, game->font_small, "Searching for opponent...", 330, 300, white);

    // Calculate wait time
    unsigned int wait_time = (ticks - game->matchmaking_start_time) / 1000;
    char wait_msg[100];
    snprintf(wait_msg, sizeof(wait_msg), "Wait time: %u seconds", wait_time);
    render_text(renderer, game->font_small, wait_msg, 380, 340, gray);

    // ELO range info
    char elo_msg[100];
    int elo_range = 100;  // Starting range
    if(wait_time >= 120) {
        elo_range = 99999;
        snprintf(elo_msg, sizeof(elo_msg), "ELO Range: Any (120s+)");
    } else if(wait_time >= 60) {
        elo_range = 800;
        snprintf(elo_msg, sizeof(elo_msg), "ELO Range: ±800");
    } else if(wait_time >= 20) {
        elo_range = 500;
        snprintf(elo_msg, sizeof(elo_msg), "ELO Range: ±500");
    } else if(wait_time >= 10) {
        elo_range = 200;
        snprintf(elo_msg, sizeof(elo_msg), "ELO Range: ±200");
    } else {
        snprintf(elo_msg, sizeof(elo_msg), "ELO Range: ±100");
    }
    render_text(renderer, game->font_small, elo_msg, 390, 370, gray);

    // Cancel button
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int cancel_hover = (mx >= 350 && mx <= 550 && my >= 450 && my <= 500);

    render_button(renderer, game->font_small, "CANCEL", 
                 350, 450, 200, 50, red, cancel_hover, 1);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void matchmaking_screen_handle_click(GameData* game, int x, int y) {
    // Cancel button
    if(x >= 350 && x <= 550 && y >= 450 && y <= 500) {
        // Send CANCEL_MATCH to server
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "CANCEL_MATCH#");
        send(game->sockfd, buffer, strlen(buffer), 0);
        printf("CLIENT: Cancelled matchmaking\n");
        
        // Return to lobby
        game->state = STATE_LOBBY;
        game->matchmaking_active = 0;
    }
}

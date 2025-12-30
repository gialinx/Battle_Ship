// profile_screen.c - Profile screen (placeholder for future)
#include "profile_screen.h"
#include "../../ui/renderer.h"
#include "../../ui/colors.h"
#include <string.h>

void profile_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};

    // Title
    render_text(renderer, game->font, "PROFILE", 420, 50, cyan);

    // User info
    char info[256];
    snprintf(info, sizeof(info), "Username: %s", game->my_username);
    render_text(renderer, game->font_small, info, 350, 150, white);

    snprintf(info, sizeof(info), "ELO Rating: %d", game->my_elo);
    render_text(renderer, game->font_small, info, 350, 200, white);

    snprintf(info, sizeof(info), "Total Games: %d", game->total_games);
    render_text(renderer, game->font_small, info, 350, 250, white);

    snprintf(info, sizeof(info), "Wins: %d", game->wins);
    render_text(renderer, game->font_small, info, 350, 300, white);

    snprintf(info, sizeof(info), "Losses: %d", game->losses);
    render_text(renderer, game->font_small, info, 350, 350, white);

    if (game->total_games > 0) {
        float win_rate = (float)game->wins / game->total_games * 100;
        snprintf(info, sizeof(info), "Win Rate: %.1f%%", win_rate);
        render_text(renderer, game->font_small, info, 350, 400, white);
    }

    // Buttons (side by side)
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int back_hover = (mx >= 280 && mx <= 460 && my >= 550 && my <= 600);
    int logout_hover = (mx >= 480 && mx <= 660 && my >= 550 && my <= 600);

    // Back to Lobby button (left)
    render_button(renderer, game->font_small, "Back to Lobby", 280, 550, 180, 50,
                 (SDL_Color){0, 100, 200, 255}, back_hover, 1);

    // Logout button (right, red color)
    render_button(renderer, game->font_small, "Logout", 480, 550, 180, 50,
                 (SDL_Color){200, 50, 50, 255}, logout_hover, 1);

    // TODO message
    render_text(renderer, game->font_small, "(Full profile coming soon...)", 330, 480,
               (SDL_Color){150, 150, 150, 255});
}

void profile_screen_handle_click(GameData* game, int x, int y) {
    // Back to Lobby button
    if(x >= 280 && x <= 460 && y >= 550 && y <= 600) {
        game->state = STATE_LOBBY;
    }

    // Logout button
    if(x >= 480 && x <= 660 && y >= 550 && y <= 600) {
        game->logout_requested = 1;
    }
}

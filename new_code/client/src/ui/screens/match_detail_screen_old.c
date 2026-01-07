// match_detail_screen.c - Detailed shot-by-shot replay
#include "match_detail_screen.h"
#include "../colors.h"
#include "../renderer.h"
#include <stdio.h>
#include <string.h>

#define HEADER_HEIGHT 80
#define BACK_BTN_X 50
#define BACK_BTN_Y 20
#define BACK_BTN_W 150
#define BACK_BTN_H 40

void match_detail_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_Rect bg = {0, 0, 1000, 700};
    SDL_RenderFillRect(renderer, &bg);
    
    // Colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {150, 150, 150, 255};
    SDL_Color green = {50, 255, 100, 255};
    SDL_Color red = {255, 100, 100, 255};
    SDL_Color cyan = {100, 200, 255, 255};
    SDL_Color yellow = {255, 220, 80, 255};
    
    MatchDetail* detail = &game->current_match_detail;
    
    // Header
    SDL_SetRenderDrawColor(renderer, 30, 45, 70, 255);
    SDL_Rect header = {0, 0, 1000, HEADER_HEIGHT};
    SDL_RenderFillRect(renderer, &header);
    
    char title[256];
    snprintf(title, sizeof(title), "MATCH DETAIL - %s", detail->date);
    render_text(renderer, game->font, title, 250, 15, white);
    
    char vs_text[256];
    snprintf(vs_text, sizeof(vs_text), "%s vs %s", detail->my_name, detail->opponent_name);
    render_text(renderer, game->font_small, vs_text, 350, 50, cyan);
    
    // Back button
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int back_hover = (mx >= BACK_BTN_X && mx <= BACK_BTN_X + BACK_BTN_W &&
                      my >= BACK_BTN_Y && my <= BACK_BTN_Y + BACK_BTN_H);
    
    SDL_Rect back_btn = {BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H};
    SDL_SetRenderDrawColor(renderer, back_hover ? 80 : 60, back_hover ? 100 : 80, back_hover ? 120 : 100, 255);
    SDL_RenderFillRect(renderer, &back_btn);
    SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
    SDL_RenderDrawRect(renderer, &back_btn);
    render_text_centered(renderer, game->font_small, "< BACK TO HISTORY", BACK_BTN_X + BACK_BTN_W/2, BACK_BTN_Y + 12, white);
    
    // Check if we have shot data
    if(detail->my_shot_count == 0 && detail->opponent_shot_count == 0) {
        // No data available
        render_text_centered(renderer, game->font, "No detailed shot data available", 500, 350, gray);
        return;
    }
    
    // Two columns for shot history
    int left_x = 50;
    int right_x = 520;
    int y = HEADER_HEIGHT + 20;
    
    // My shots column
    render_text(renderer, game->font_small, "YOUR SHOTS:", left_x, y, cyan);
    y += 30;
    
    int my_y = y;
    for(int i = 0; i < detail->my_shot_count && i < 20; i++) {
        ShotEntry* shot = &detail->my_shots[i];
        
        char shot_text[256];
        char coord[10];
        snprintf(coord, sizeof(coord), "%c%d", 'A' + shot->x, shot->y + 1);
        
        if(shot->hit) {
            if(shot->ship_sunk) {
                snprintf(shot_text, sizeof(shot_text), "#%d %s - HIT! Sunk ship (len %d)", 
                        i+1, coord, shot->ship_length);
                render_text(renderer, game->font_small, shot_text, left_x, my_y, yellow);
            } else {
                snprintf(shot_text, sizeof(shot_text), "#%d %s - HIT ship (len %d)", 
                        i+1, coord, shot->ship_length);
                render_text(renderer, game->font_small, shot_text, left_x, my_y, green);
            }
        } else {
            snprintf(shot_text, sizeof(shot_text), "#%d %s - MISS", i+1, coord);
            render_text(renderer, game->font_small, shot_text, left_x, my_y, gray);
        }
        
        my_y += 25;
    }
    
    // Opponent shots column
    render_text(renderer, game->font_small, "OPPONENT SHOTS:", right_x, y, red);
    
    int opp_y = y + 30;
    for(int i = 0; i < detail->opponent_shot_count && i < 20; i++) {
        ShotEntry* shot = &detail->opponent_shots[i];
        
        char shot_text[256];
        char coord[10];
        snprintf(coord, sizeof(coord), "%c%d", 'A' + shot->x, shot->y + 1);
        
        if(shot->hit) {
            if(shot->ship_sunk) {
                snprintf(shot_text, sizeof(shot_text), "#%d %s - HIT! Sunk ship (len %d)", 
                        i+1, coord, shot->ship_length);
                render_text(renderer, game->font_small, shot_text, right_x, opp_y, yellow);
            } else {
                snprintf(shot_text, sizeof(shot_text), "#%d %s - HIT ship (len %d)", 
                        i+1, coord, shot->ship_length);
                render_text(renderer, game->font_small, shot_text, right_x, opp_y, green);
            }
        } else {
            snprintf(shot_text, sizeof(shot_text), "#%d %s - MISS", i+1, coord);
            render_text(renderer, game->font_small, shot_text, right_x, opp_y, gray);
        }
        
        opp_y += 25;
    }
    
    // Result at bottom
    y = 650;
    char result_text[256];
    if(detail->winner == 1) {
        snprintf(result_text, sizeof(result_text), "YOU WON!");
        render_text_centered(renderer, game->font, result_text, 500, y, green);
    } else {
        snprintf(result_text, sizeof(result_text), "YOU LOST");
        render_text_centered(renderer, game->font, result_text, 500, y, red);
    }
}

void match_detail_screen_handle_click(int mouse_x, int mouse_y, GameData* game) {
    // Back button
    if(mouse_x >= BACK_BTN_X && mouse_x <= BACK_BTN_X + BACK_BTN_W &&
       mouse_y >= BACK_BTN_Y && mouse_y <= BACK_BTN_Y + BACK_BTN_H) {
        game->state = STATE_MATCH_HISTORY;
        return;
    }
}

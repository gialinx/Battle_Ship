// match_history_screen.c - Match history list with detailed information
#include "match_history_screen.h"
#include "../colors.h"
#include "../renderer.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

extern void send_msg(const char* msg);

#define HEADER_HEIGHT 80
#define ENTRY_HEIGHT 100
#define BACK_BTN_X 50
#define BACK_BTN_Y 20
#define BACK_BTN_W 120
#define BACK_BTN_H 40

void match_history_screen_render(SDL_Renderer* renderer, GameData* game) {
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
    
    // Header
    SDL_SetRenderDrawColor(renderer, 30, 45, 70, 255);
    SDL_Rect header = {0, 0, 1000, HEADER_HEIGHT};
    SDL_RenderFillRect(renderer, &header);
    
    render_text(renderer, game->font, "MATCH HISTORY", 350, 25, white);
    
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
    render_text_centered(renderer, game->font_small, "< BACK", BACK_BTN_X + BACK_BTN_W/2, BACK_BTN_Y + 12, white);
    
    // Column headers with fixed widths
    int y = HEADER_HEIGHT + 10;
    render_text(renderer, game->font_small, "DATE", 20, y, gray);
    render_text(renderer, game->font_small, "OPPONENT", 150, y, gray);
    render_text(renderer, game->font_small, "RESULT", 320, y, gray);
    render_text(renderer, game->font_small, "HIT", 420, y, gray);
    render_text(renderer, game->font_small, "MISS", 480, y, gray);
    render_text(renderer, game->font_small, "ACC%", 550, y, gray);
    render_text(renderer, game->font_small, "ELO", 630, y, gray);
    render_text(renderer, game->font_small, "TIME", 740, y, gray);
    render_text(renderer, game->font_small, "VIEW", 850, y, gray);
    
    y += 30;
    
    // Match entries
    for(int i = 0; i < game->match_history_count && i < 5; i++) {
        MatchHistoryEntry* m = &game->match_history[i];
        
        // Entry background
        SDL_Rect entry = {10, y, 980, ENTRY_HEIGHT};
        int entry_hover = (mx >= entry.x && mx <= entry.x + entry.w &&
                          my >= entry.y && my <= entry.y + entry.h);
        
        SDL_SetRenderDrawColor(renderer, entry_hover ? 45 : 35, entry_hover ? 55 : 45, entry_hover ? 75 : 65, 255);
        SDL_RenderFillRect(renderer, &entry);
        SDL_SetRenderDrawColor(renderer, 70, 80, 100, 255);
        SDL_RenderDrawRect(renderer, &entry);
        
        // Date (compact format)
        char date_compact[20];
        if(strlen(m->date) >= 16) {
            snprintf(date_compact, sizeof(date_compact), "%c%c/%c%c %c%c:%c%c",
                    m->date[5], m->date[6], m->date[8], m->date[9],
                    m->date[11], m->date[12], m->date[14], m->date[15]);
        } else {
            strcpy(date_compact, "--/-- --:--");
        }
        render_text(renderer, game->font_small, date_compact, 20, y + 25, white);
        
        // Opponent
        char opponent_text[20];
        snprintf(opponent_text, sizeof(opponent_text), "%.12s", m->opponent_name);
        render_text(renderer, game->font_small, opponent_text, 150, y + 15, cyan);
        char opp_id[20];
        snprintf(opp_id, sizeof(opp_id), "ID:%d", m->opponent_id);
        render_text(renderer, game->font_small, opp_id, 150, y + 35, gray);
        
        // Result
        SDL_Color result_color = m->result ? green : red;
        render_text(renderer, game->font_small, m->result ? "WIN" : "LOSS", 320, y + 25, result_color);
        
        // HIT
        char hit_text[10];
        snprintf(hit_text, sizeof(hit_text), "%d", m->my_hits);
        render_text(renderer, game->font_small, hit_text, 425, y + 25, green);
        
        // MISS
        char miss_text[10];
        snprintf(miss_text, sizeof(miss_text), "%d", m->my_misses);
        render_text(renderer, game->font_small, miss_text, 485, y + 25, red);
        
        // Accuracy %
        char acc_text[10];
        int total_shots = m->my_hits + m->my_misses;
        float accuracy = total_shots > 0 ? (float)m->my_hits * 100 / total_shots : 0;
        snprintf(acc_text, sizeof(acc_text), "%.0f%%", accuracy);
        render_text(renderer, game->font_small, acc_text, 555, y + 25, white);
        
        // ELO change
        char elo_text[15];
        snprintf(elo_text, sizeof(elo_text), "%+d", m->elo_change);
        SDL_Color elo_color = m->elo_change > 0 ? green : red;
        render_text(renderer, game->font_small, elo_text, 635, y + 15, elo_color);
        
        char elo_detail[30];
        snprintf(elo_detail, sizeof(elo_detail), "%d->%d", m->my_elo_before, m->my_elo_after);
        render_text(renderer, game->font_small, elo_detail, 635, y + 35, gray);
        
        // Duration
        int minutes = m->duration_seconds / 60;
        int seconds = m->duration_seconds % 60;
        char time_text[15];
        snprintf(time_text, sizeof(time_text), "%d:%02d", minutes, seconds);
        render_text(renderer, game->font_small, time_text, 745, y + 25, white);
        
        // View Details button
        SDL_Rect details_btn = {840, y + 20, 100, 30};
        int details_hover = (mx >= details_btn.x && mx <= details_btn.x + details_btn.w &&
                            my >= details_btn.y && my <= details_btn.y + details_btn.h);
        
        SDL_SetRenderDrawColor(renderer, details_hover ? 70 : 50, details_hover ? 120 : 100, details_hover ? 180 : 150, 255);
        SDL_RenderFillRect(renderer, &details_btn);
        SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
        SDL_RenderDrawRect(renderer, &details_btn);
        render_text_centered(renderer, game->font_small, ">> VIEW", details_btn.x + details_btn.w/2, details_btn.y + 8, white);
        
        y += ENTRY_HEIGHT + 10;
    }
    
    // Empty state
    if(game->match_history_count == 0) {
        render_text_centered(renderer, game->font, "No match history yet", 500, 350, gray);
    }
}

void match_history_screen_handle_click(int mouse_x, int mouse_y, GameData* game) {
    // Back button
    if(mouse_x >= BACK_BTN_X && mouse_x <= BACK_BTN_X + BACK_BTN_W &&
       mouse_y >= BACK_BTN_Y && mouse_y <= BACK_BTN_Y + BACK_BTN_H) {
        game->state = STATE_LOBBY;
        return;
    }
    
    // View Details buttons
    int y = HEADER_HEIGHT + 40;
    for(int i = 0; i < game->match_history_count && i < 5; i++) {
        SDL_Rect details_btn = {840, y + 20, 100, 30};
        
        if(mouse_x >= details_btn.x && mouse_x <= details_btn.x + details_btn.w &&
           mouse_y >= details_btn.y && mouse_y <= details_btn.y + details_btn.h) {
            
            // Request match details from server
            game->viewing_match_id = game->match_history[i].match_id;
            char msg[64];
            snprintf(msg, sizeof(msg), "GET_MATCH_DETAIL:%d", game->viewing_match_id);
            send_msg(msg);
            
            printf("Requesting match detail for match_id=%d\n", game->viewing_match_id);
            return;
        }
        
        y += ENTRY_HEIGHT + 10;
    }
}

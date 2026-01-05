// lobby_screen.c - NEW REDESIGNED LOBBY
#include "lobby_screen.h"
#include "../../ui/renderer.h"
#include "../../ui/colors.h"
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700
#define HEADER_HEIGHT 80
#define PLAYER_LIST_WIDTH 300

// ==================== RENDER FUNCTIONS ====================

// Vẽ header bar (logo + find match + user info)
void render_header(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color green = {0, 200, 0, 255};

    // Background header
    SDL_SetRenderDrawColor(renderer, 30, 40, 60, 255);
    SDL_Rect header_bg = {0, 0, SCREEN_WIDTH, HEADER_HEIGHT};
    SDL_RenderFillRect(renderer, &header_bg);

    // Logo (placeholder text)
    render_text(renderer, game->font, "BATTLESHIP", 20, 25, cyan);

    // Find Match button
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int find_match_hover = (mx >= 200 && mx <= 400 && my >= 20 && my <= 60);

    render_button(renderer, game->font_small, "FIND MATCH",
                 200, 20, 200, 40, green, find_match_hover, 1);
    
    // Match History button
    // int history_hover = (mx >= 410 && mx <= 590 && my >= 20 && my <= 60);
    // SDL_Color purple = {150, 100, 255, 255};
    // render_button(renderer, game->font_small, "MATCH HISTORY",
    //              410, 20, 180, 40, purple, history_hover, 1);

    // User info (right side)
    char user_info[256];
    snprintf(user_info, sizeof(user_info), "%s", game->my_username);
    render_text(renderer, game->font_small, user_info, 750, 15, white);

    snprintf(user_info, sizeof(user_info), "ELO: %d", game->my_elo);
    render_text(renderer, game->font_small, user_info, 750, 45, cyan);

    // Avatar placeholder (circle)
    SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
    for(int w = 0; w < 50; w++) {
        for(int h = 0; h < 50; h++) {
            int dx = w - 25;
            int dy = h - 25;
            if((dx*dx + dy*dy) <= (25*25)) {
                SDL_RenderDrawPoint(renderer, 920 + w, 15 + h);
            }
        }
    }
}

// Vẽ tabs (Leaderboard / Stats / History)
void render_tabs(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color active_color = {0, 100, 200, 255};
    SDL_Color inactive_color = {60, 70, 90, 255};

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    const char* tab_names[] = {"Leaderboard", "Stats", "History"};
    int tab_width = 150;
    int tab_height = 40;
    int start_x = 20;
    int start_y = HEADER_HEIGHT + 10;

    for(int i = 0; i < 3; i++) {
        int x = start_x + i * (tab_width + 10);
        int is_active = (game->active_lobby_tab == i);
        int is_hover = (mx >= x && mx <= x + tab_width &&
                       my >= start_y && my <= start_y + tab_height);

        SDL_Color bg_color = is_active ? active_color : inactive_color;

        SDL_Rect tab_rect = {x, start_y, tab_width, tab_height};
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, 255);
        SDL_RenderFillRect(renderer, &tab_rect);

        if(is_hover && !is_active) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(renderer, &tab_rect);
        }

        render_text(renderer, game->font_small, tab_names[i], x + 20, start_y + 10, white);
    }
}

// Vẽ main content area (dựa theo tab được chọn)
void render_main_content(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {150, 150, 150, 255};

    int content_x = 20;
    int content_y = HEADER_HEIGHT + 60;
    int content_width = SCREEN_WIDTH - PLAYER_LIST_WIDTH - 40;
    int content_height = SCREEN_HEIGHT - content_y - 20;

    // Background
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_Rect content_bg = {content_x, content_y, content_width, content_height};
    SDL_RenderFillRect(renderer, &content_bg);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &content_bg);

    // Render based on active tab
    if(game->active_lobby_tab == LOBBY_TAB_LEADERBOARD) {
        render_text(renderer, game->font, "TOP 10 PLAYERS", content_x + 180, content_y + 20, white);

        if(game->leaderboard_count == 0) {
            render_text(renderer, game->font_small, "Loading leaderboard...",
                       content_x + 200, content_y + 80, gray);
        } else {
            // Header row
            int header_y = content_y + 70;
            render_text(renderer, game->font_small, "Rank", content_x + 30, header_y, gray);
            render_text(renderer, game->font_small, "Username", content_x + 100, header_y, gray);
            render_text(renderer, game->font_small, "ELO", content_x + 300, header_y, gray);
            render_text(renderer, game->font_small, "Games", content_x + 400, header_y, gray);
            render_text(renderer, game->font_small, "Wins", content_x + 500, header_y, gray);

            // Entries
            int entry_y = header_y + 40;
            for(int i = 0; i < game->leaderboard_count && i < 10; i++) {
                LeaderboardEntry* entry = &game->leaderboard[i];
                char text[256];

                SDL_Color rank_color = white;
                if(i == 0) rank_color = (SDL_Color){255, 215, 0, 255};      // Gold
                else if(i == 1) rank_color = (SDL_Color){192, 192, 192, 255}; // Silver
                else if(i == 2) rank_color = (SDL_Color){205, 127, 50, 255}; // Bronze

                snprintf(text, sizeof(text), "#%d", entry->rank);
                render_text(renderer, game->font_small, text, content_x + 30, entry_y, rank_color);

                render_text(renderer, game->font_small, entry->username, content_x + 100, entry_y, white);

                snprintf(text, sizeof(text), "%d", entry->elo_rating);
                render_text(renderer, game->font_small, text, content_x + 300, entry_y, white);

                snprintf(text, sizeof(text), "%d", entry->total_games);
                render_text(renderer, game->font_small, text, content_x + 400, entry_y, white);

                snprintf(text, sizeof(text), "%d", entry->wins);
                render_text(renderer, game->font_small, text, content_x + 500, entry_y, white);

                entry_y += 35;
            }
        }

    } else if(game->active_lobby_tab == LOBBY_TAB_STATS) {
        render_text(renderer, game->font, "YOUR STATISTICS", content_x + 200, content_y + 20, white);

        char stat[256];
        int y = content_y + 80;

        snprintf(stat, sizeof(stat), "Total Games: %d", game->total_games);
        render_text(renderer, game->font_small, stat, content_x + 50, y, white);
        y += 40;

        snprintf(stat, sizeof(stat), "Wins: %d", game->wins);
        render_text(renderer, game->font_small, stat, content_x + 50, y, white);
        y += 40;

        snprintf(stat, sizeof(stat), "Losses: %d", game->losses);
        render_text(renderer, game->font_small, stat, content_x + 50, y, white);
        y += 40;

        if(game->total_games > 0) {
            float win_rate = (float)game->wins / game->total_games * 100;
            snprintf(stat, sizeof(stat), "Win Rate: %.1f%%", win_rate);
            render_text(renderer, game->font_small, stat, content_x + 50, y, white);
        }

    } else if(game->active_lobby_tab == LOBBY_TAB_HISTORY) {
        render_text(renderer, game->font, "MATCH HISTORY", content_x + 180, content_y + 10, white);
        
        // Display match history entries
        if(game->match_history_count == 0) {
            render_text(renderer, game->font_small, "No matches played yet",
                       content_x + 180, content_y + 150, gray);
        } else {
            // Column headers background
            SDL_Rect header_bg = {content_x + 10, content_y + 55, 580, 30};
            SDL_SetRenderDrawColor(renderer, 35, 45, 65, 255);
            SDL_RenderFillRect(renderer, &header_bg);
            
            // Column headers
            int header_y = content_y + 62;
            render_text(renderer, game->font_small, "DATE", content_x + 15, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "OPPONENT", content_x + 85, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "RESULT", content_x + 195, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "HIT", content_x + 270, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "MISS", content_x + 325, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "ACC%", content_x + 385, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "ELO", content_x + 450, header_y, (SDL_Color){200, 200, 200, 255});
            render_text(renderer, game->font_small, "VIEW", content_x + 530, header_y, (SDL_Color){200, 200, 200, 255});
            
            // Get mouse position for hover
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            
            // Match entries (show up to 7 matches)
            int entry_y = content_y + 92;
            for(int i = 0; i < game->match_history_count && i < 7; i++) {
                MatchHistoryEntry* m = &game->match_history[i];
                
                // Row background (alternating colors)
                SDL_Rect row_bg = {content_x + 10, entry_y - 5, 580, 40};
                int is_hover = (mx >= row_bg.x && mx <= row_bg.x + row_bg.w &&
                               my >= row_bg.y && my <= row_bg.y + row_bg.h);
                
                if(is_hover) {
                    SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);
                } else if(i % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 30, 40, 60, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 25, 35, 55, 255);
                }
                SDL_RenderFillRect(renderer, &row_bg);
                
                // Border line
                SDL_SetRenderDrawColor(renderer, 50, 60, 80, 255);
                SDL_RenderDrawRect(renderer, &row_bg);
                
                // Date (short format MM-DD HH:MM)
                char date_short[20];
                if(strlen(m->date) >= 16) {
                    snprintf(date_short, sizeof(date_short), "%c%c-%c%c %c%c:%c%c",
                            m->date[5], m->date[6], m->date[8], m->date[9],
                            m->date[11], m->date[12], m->date[14], m->date[15]);
                } else {
                    strcpy(date_short, "--");
                }
                render_text(renderer, game->font_small, date_short, content_x + 15, entry_y, white);
                
                // Opponent
                char opp[20];
                snprintf(opp, sizeof(opp), "%.10s", m->opponent_name);
                render_text(renderer, game->font_small, opp, content_x + 85, entry_y, (SDL_Color){100, 200, 255, 255});
                
                // Result
                SDL_Color result_color = m->result ? (SDL_Color){50, 255, 100, 255} : (SDL_Color){255, 100, 100, 255};
                render_text(renderer, game->font_small, m->result ? "WIN" : "LOSS", 
                           content_x + 195, entry_y, result_color);
                
                // HIT
                char hit_str[10];
                snprintf(hit_str, sizeof(hit_str), "%d", m->my_hits);
                render_text(renderer, game->font_small, hit_str, content_x + 275, entry_y, (SDL_Color){100, 255, 150, 255});
                
                // MISS
                char miss_str[10];
                snprintf(miss_str, sizeof(miss_str), "%d", m->my_misses);
                render_text(renderer, game->font_small, miss_str, content_x + 330, entry_y, (SDL_Color){255, 150, 100, 255});
                
                // Accuracy %
                int total = m->my_hits + m->my_misses;
                float acc = total > 0 ? (float)m->my_hits * 100 / total : 0;
                char acc_str[10];
                snprintf(acc_str, sizeof(acc_str), "%.0f%%", acc);
                render_text(renderer, game->font_small, acc_str, content_x + 385, entry_y, (SDL_Color){200, 200, 100, 255});
                
                // ELO change
                char elo[20];
                snprintf(elo, sizeof(elo), "%+d", m->elo_change);
                SDL_Color elo_color = m->elo_change > 0 ? (SDL_Color){50, 255, 100, 255} : (SDL_Color){255, 100, 100, 255};
                render_text(renderer, game->font_small, elo, content_x + 450, entry_y, elo_color);
                
                // View Details button with icon
                SDL_Rect detail_btn = {content_x + 520, entry_y - 3, 50, 25};
                int btn_hover = (mx >= detail_btn.x && mx <= detail_btn.x + detail_btn.w &&
                                my >= detail_btn.y && my <= detail_btn.y + detail_btn.h);
                
                SDL_SetRenderDrawColor(renderer, 
                                      btn_hover ? 80 : 60,
                                      btn_hover ? 120 : 100,
                                      btn_hover ? 200 : 180, 255);
                SDL_RenderFillRect(renderer, &detail_btn);
                SDL_SetRenderDrawColor(renderer, 100, 150, 220, 255);
                SDL_RenderDrawRect(renderer, &detail_btn);
                // Use simple ASCII arrow instead of emoji
                render_text_centered(renderer, game->font_small, ">>", 
                                   detail_btn.x + detail_btn.w/2, detail_btn.y + 6, white);
                
                entry_y += 45;
            }
        }
    }
}

// Vẽ player list (bên phải)
void render_player_list(SDL_Renderer* renderer, GameData* game) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color gray = {150, 150, 150, 255};

    int list_x = SCREEN_WIDTH - PLAYER_LIST_WIDTH;
    int list_y = HEADER_HEIGHT;

    // Background
    SDL_SetRenderDrawColor(renderer, 35, 45, 65, 255);
    SDL_Rect list_bg = {list_x, list_y, PLAYER_LIST_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT};
    SDL_RenderFillRect(renderer, &list_bg);

    // Title
    render_text(renderer, game->font_small, "ONLINE PLAYERS", list_x + 50, list_y + 10, white);

    // Search box
    SDL_SetRenderDrawColor(renderer, 60, 70, 90, 255);
    SDL_Rect search_box = {list_x + 10, list_y + 40, PLAYER_LIST_WIDTH - 20, 30};
    SDL_RenderFillRect(renderer, &search_box);

    // Border color - highlight if active
    if(game->player_search_field.is_active) {
        SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    SDL_RenderDrawRect(renderer, &search_box);

    // Display search text or placeholder
    if(strlen(game->player_search_field.text) > 0) {
        render_text(renderer, game->font_small, game->player_search_field.text, list_x + 20, list_y + 45, white);
    } else {
        render_text(renderer, game->font_small, "Search...", list_x + 20, list_y + 45, gray);
    }

    // Player list with filtering
    int y = list_y + 90;
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // Convert search text to lowercase for case-insensitive search
    char search_lower[50];
    strncpy(search_lower, game->player_search_field.text, sizeof(search_lower) - 1);
    search_lower[sizeof(search_lower) - 1] = '\0';
    for(int i = 0; search_lower[i]; i++) {
        if(search_lower[i] >= 'A' && search_lower[i] <= 'Z') {
            search_lower[i] = search_lower[i] + 32;  // Convert to lowercase
        }
    }

    int displayed_count = 0;
    for(int i = 0; i < game->user_count && displayed_count < 8; i++) {
        UserInfo* u = &game->users[i];
        if(u->user_id == game->my_user_id) continue;

        // Filter by search text (case-insensitive)
        if(strlen(search_lower) > 0) {
            char username_lower[50];
            strncpy(username_lower, u->username, sizeof(username_lower) - 1);
            username_lower[sizeof(username_lower) - 1] = '\0';
            for(int j = 0; username_lower[j]; j++) {
                if(username_lower[j] >= 'A' && username_lower[j] <= 'Z') {
                    username_lower[j] = username_lower[j] + 32;
                }
            }

            // Check if search text is found in username
            if(strstr(username_lower, search_lower) == NULL) {
                continue;  // Skip this user
            }
        }

        SDL_Rect player_rect = {list_x + 5, y, PLAYER_LIST_WIDTH - 10, 60};

        // Check if user is in game
        int is_busy = u->in_game;

        // Hover effect (disabled if busy)
        if(is_busy) {
            SDL_SetRenderDrawColor(renderer, 30, 35, 40, 255);  // Dark gray for busy
        } else if(mx >= player_rect.x && mx <= player_rect.x + player_rect.w &&
           my >= player_rect.y && my <= player_rect.y + player_rect.h) {
            SDL_SetRenderDrawColor(renderer, 60, 80, 110, 255);  // Hover
        } else {
            SDL_SetRenderDrawColor(renderer, 50, 60, 80, 255);  // Normal
        }
        SDL_RenderFillRect(renderer, &player_rect);
        
        if(is_busy) {
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);  // Darker border
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        }
        SDL_RenderDrawRect(renderer, &player_rect);

        // Player info
        SDL_Color name_color = is_busy ? (SDL_Color){120, 120, 120, 255} : white;
        render_text(renderer, game->font_small, u->username, list_x + 15, y + 5, name_color);

        char elo_text[50];
        snprintf(elo_text, sizeof(elo_text), "ELO: %d", u->elo_rating);
        SDL_Color elo_color = is_busy ? (SDL_Color){90, 90, 90, 255} : gray;
        render_text(renderer, game->font_small, elo_text, list_x + 15, y + 25, elo_color);

        // Status
        if(is_busy) {
            SDL_Color busy_color = {255, 150, 50, 255};  // Orange
            render_text(renderer, game->font_small, "In Game", list_x + 200, y + 15, busy_color);
        } else {
            SDL_Color status_color = strcmp(u->status, "online") == 0 ? green : gray;
            render_text(renderer, game->font_small, u->status, list_x + 200, y + 25, status_color);
        }

        y += 65;
        displayed_count++;
    }

    // Show "No results" if search yielded nothing
    if(displayed_count == 0 && strlen(game->player_search_field.text) > 0) {
        render_text(renderer, game->font_small, "No players found", list_x + 70, y, gray);
    }
}

// ==================== MAIN RENDER ====================
void lobby_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);

    render_header(renderer, game);
    render_tabs(renderer, game);
    render_main_content(renderer, game);
    render_player_list(renderer, game);
}

// ==================== HANDLE CLICKS ====================
void lobby_screen_handle_click(GameData* game, int x, int y) {
    // Avatar click (go to profile)
    if(x >= 920 && x <= 970 && y >= 15 && y <= 65) {
        game->state = STATE_PROFILE;
        return;
    }

    // Find Match button
    if(x >= 250 && x <= 450 && y >= 20 && y <= 60) {
        // Start matchmaking
        printf("Find Match clicked!\n");
        
        game->state = STATE_MATCHMAKING;
        game->matchmaking_active = 1;
        game->matchmaking_start_time = SDL_GetTicks();
        
        // Send FIND_MATCH to server
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "FIND_MATCH#");
        send(game->sockfd, buffer, strlen(buffer), 0);
        printf("CLIENT: Sent FIND_MATCH to server\n");
        return;
    }
    // Tabs
    int tab_y = HEADER_HEIGHT + 10;
    if(y >= tab_y && y <= tab_y + 40) {
        for(int i = 0; i < 3; i++) {
            int tab_x = 20 + i * 160;
            if(x >= tab_x && x <= tab_x + 150) {
                game->active_lobby_tab = i;
                
                // If switching to History tab, request match history
                if(i == LOBBY_TAB_HISTORY) {
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "GET_MATCH_HISTORY#");
                    send(game->sockfd, buffer, strlen(buffer), 0);
                    printf("CLIENT: Switched to History tab, requesting match history\n");
                }
                
                return;
            }
        }
    }
    
    // Details button in History tab
    if(game->active_lobby_tab == LOBBY_TAB_HISTORY) {
        int content_x = 29;
        int content_y = 155;
        int entry_y = content_y + 92;
        
        for(int i = 0; i < game->match_history_count && i < 7; i++) {
            SDL_Rect detail_btn = {content_x + 520, entry_y - 3, 50, 25};
            if(x >= detail_btn.x && x <= detail_btn.x + detail_btn.w &&
               y >= detail_btn.y && y <= detail_btn.y + detail_btn.h) {
                // Send request for match details
                MatchHistoryEntry* m = &game->match_history[i];
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "GET_MATCH_DETAIL:%d#", m->match_id);
                send(game->sockfd, buffer, strlen(buffer), 0);
                printf("CLIENT: Requesting details for match %d\n", m->match_id);
                return;
            }
            entry_y += 45;
        }
    }

    // Search box click
    int list_x = SCREEN_WIDTH - PLAYER_LIST_WIDTH;
    int search_y = HEADER_HEIGHT + 40;
    if(x >= list_x + 10 && x <= list_x + PLAYER_LIST_WIDTH - 10 &&
       y >= search_y && y <= search_y + 30) {
        game->player_search_field.is_active = 1;
        SDL_StartTextInput();
        return;
    }

    // Player list clicks (with same filtering as render)
    int list_y = HEADER_HEIGHT + 90;

    // Convert search text to lowercase for filtering
    char search_lower[50];
    strncpy(search_lower, game->player_search_field.text, sizeof(search_lower) - 1);
    search_lower[sizeof(search_lower) - 1] = '\0';
    for(int i = 0; search_lower[i]; i++) {
        if(search_lower[i] >= 'A' && search_lower[i] <= 'Z') {
            search_lower[i] = search_lower[i] + 32;
        }
    }

    int displayed_count = 0;
    for(int i = 0; i < game->user_count && displayed_count < 8; i++) {
        UserInfo* u = &game->users[i];
        if(u->user_id == game->my_user_id) continue;

        // Apply same filter as render
        if(strlen(search_lower) > 0) {
            char username_lower[50];
            strncpy(username_lower, u->username, sizeof(username_lower) - 1);
            username_lower[sizeof(username_lower) - 1] = '\0';
            for(int j = 0; username_lower[j]; j++) {
                if(username_lower[j] >= 'A' && username_lower[j] <= 'Z') {
                    username_lower[j] = username_lower[j] + 32;
                }
            }

            if(strstr(username_lower, search_lower) == NULL) {
                continue;  // Skip filtered users
            }
        }

        if(x >= list_x + 5 && x <= list_x + PLAYER_LIST_WIDTH - 5 &&
           y >= list_y && y <= list_y + 60) {
            
            // Check if user is busy in game
            if(u->in_game) {
                snprintf(game->message, sizeof(game->message), 
                        "%s is busy in game. Cannot invite!", u->username);
                printf("CLIENT: Cannot invite %s - player is in game\n", u->username);
                return;
            }
            
            if(strcmp(u->status, "online") == 0) {
                game->invited_user_id = u->user_id;
                strcpy(game->invited_username, u->username);
                game->state = STATE_SENDING_INVITE;
                
                // Send INVITE command to server
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "INVITE:%d#", u->user_id);
                send(game->sockfd, buffer, strlen(buffer), 0);
                printf("CLIENT: Sent INVITE to user_id=%d (%s)\n", u->user_id, u->username);
            }
            return;
        }
        list_y += 65;
        displayed_count++;
    }

    // Deactivate search if clicking outside the player list panel area
    if(x < list_x || x > list_x + PLAYER_LIST_WIDTH) {
        game->player_search_field.is_active = 0;
    }
}

// Keep old invite dialog functions for compatibility
void lobby_screen_render_invite_dialog(SDL_Renderer* renderer, GameData* game) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_Rect dialog = {250, 200, 500, 300};
    SDL_SetRenderDrawColor(renderer, 40, 50, 70, 255);
    SDL_RenderFillRect(renderer, &dialog);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    int close_x = 720, close_y = 205, close_size = 25;
    int close_hover = (mx >= close_x && mx <= close_x + close_size &&
                       my >= close_y && my <= close_y + close_size);

    SDL_Rect close_btn = {close_x, close_y, close_size, close_size};
    SDL_SetRenderDrawColor(renderer, close_hover ? 255 : 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &close_btn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &close_btn);
    render_text(renderer, game->font_small, "X", close_x + 7, close_y + 2, white);

    if(game->state == STATE_SENDING_INVITE) {
        render_text(renderer, game->font, "Sending invitation...", 320, 250, cyan);
        char msg[256];
        snprintf(msg, sizeof(msg), "Sending invite to %s", game->invited_username);
        render_text(renderer, game->font_small, msg, 300, 300, white);

        // Simple spinner animation
        unsigned int ticks = SDL_GetTicks();
        int spinner_frame = (ticks / 200) % 4;
        const char* spinner_chars[] = {"|", "/", "-", "\\"};
        render_text(renderer, game->font, spinner_chars[spinner_frame], 480, 350, cyan);
    }
    else if(game->state == STATE_WAITING_INVITE) {
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
        snprintf(msg, sizeof(msg), "%s wants to play with you", game->inviter_username);
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
    int close_x = 720, close_y = 205, close_size = 25;
    if(x >= close_x && x <= close_x + close_size && y >= close_y && y <= close_y + close_size) {
        game->state = STATE_LOBBY;
        return;
    }

    if(game->state == STATE_WAITING_INVITE) {
        if(x >= 400 && x <= 600 && y >= 400 && y <= 450) {
            game->state = STATE_LOBBY;
        }
    }
    else if(game->state == STATE_RECEIVED_INVITE) {
        if(x >= 280 && x <= 460 && y >= 400 && y <= 450) {
            // Accept handled by main client
        }
        else if(x >= 480 && x <= 660 && y >= 400 && y <= 450) {
            game->state = STATE_LOBBY;
        }
    }
}

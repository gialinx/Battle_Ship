// protocol.c - Protocol parsing implementation
#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Forward declaration
void placing_ships_init(GameData* game);

// ==================== PARSE STATE MESSAGE ====================
int parse_state_message(GameData* game, const char* state_data) {
    if(!state_data || strncmp(state_data, "STATE:", 6) != 0) {
        return 0;
    }
    
    const char* state_content = state_data + 6;  // Skip "STATE:"
    const char *own_start = strstr(state_content, "OWN MAP");
    const char *enemy_start = strstr(state_content, "ENEMY MAP");
    
    if(!own_start || !enemy_start) {
        printf("DEBUG: Failed to find OWN MAP or ENEMY MAP in STATE\n");
        return 0;
    }
    
    // Skip "OWN MAP\n" to get to the first row
    own_start = strchr(own_start, '\n');
    if(!own_start) return 0;
    own_start++; // Skip the newline
    
    // Skip "ENEMY MAP\n" 
    enemy_start = strchr(enemy_start, '\n');
    if(!enemy_start) return 0;
    enemy_start++;
    
    // Find where ENEMY MAP starts to know where OWN MAP ends
    const char *enemy_marker = strstr(own_start, "ENEMY MAP");
    if(!enemy_marker) return 0;
    
    char own_copy[BUFF_SIZE];
    int copy_len = enemy_marker - own_start;
    if(copy_len >= BUFF_SIZE) copy_len = BUFF_SIZE - 1;
    strncpy(own_copy, own_start, copy_len);
    own_copy[copy_len] = '\0';
    
    // Parse OWN MAP row by row
    char *row_tok = strtok(own_copy, "\n");
    int row = 0;
    while(row_tok && row < MAP_SIZE) {
        // Parse each character (skip spaces)
        int col = 0;
        for(int i = 0; row_tok[i] != '\0' && col < MAP_SIZE; i++) {
            if(row_tok[i] != ' ') {
                game->own_map[row][col] = row_tok[i];
                col++;
            }
        }
        row++;
        row_tok = strtok(NULL, "\n");
    }
    
    // Count ships from map - simple approach: count cells then divide by ship length
    for(int i = 2; i <= 4; i++) game->ships_placed_count[i] = 0;
    
    int cell_count[5] = {0};  // cell_count[2], [3], [4]
    
    for(int r = 0; r < MAP_SIZE; r++) {
        for(int c = 0; c < MAP_SIZE; c++) {
            char ch = game->own_map[r][c];
            if(ch >= '2' && ch <= '4') {
                int length = ch - '0';
                cell_count[length]++;
            }
        }
    }
    
    // Calculate number of ships: total cells / ship length
    for(int len = 2; len <= 4; len++) {
        game->ships_placed_count[len] = cell_count[len] / len;
    }
    
    // Parse ENEMY MAP
    char enemy_copy[BUFF_SIZE];
    const char *enemy_end = strchr(enemy_start, '#');
    if(enemy_end) {
        int len = enemy_end - enemy_start;
        if(len >= BUFF_SIZE) len = BUFF_SIZE - 1;
        strncpy(enemy_copy, enemy_start, len);
        enemy_copy[len] = '\0';
    } else {
        strncpy(enemy_copy, enemy_start, BUFF_SIZE - 1);
        enemy_copy[BUFF_SIZE - 1] = '\0';
    }
    
    row_tok = strtok(enemy_copy, "\n");
    row = 0;
    while(row_tok && row < MAP_SIZE) {
        int col = 0;
        for(int i = 0; row_tok[i] != '\0' && col < MAP_SIZE; i++) {
            if(row_tok[i] != ' ') {
                game->enemy_map[row][col] = row_tok[i];
                col++;
            }
        }
        row++;
        row_tok = strtok(NULL, "\n");
    }
    
    return 1;
}

// ==================== PARSE LOGIN RESPONSE ====================
int parse_login_response(GameData* game, const char* msg) {
    if(strncmp(msg, "LOGIN_OK:", 9) == 0) {
        sscanf(msg, "LOGIN_OK:%[^:]:%d:%d:%d:%d",
               game->my_username, &game->total_games, &game->wins,
               &game->my_elo, &game->my_user_id);
        return 1;
    }
    
    // Handle MY_STATS response (same format as LOGIN_OK)
    if(strncmp(msg, "MY_STATS:", 9) == 0) {
        sscanf(msg, "MY_STATS:%[^:]:%d:%d:%d:%d:%d",
               game->my_username, &game->total_games, &game->wins,
               &game->losses, &game->my_elo, &game->my_user_id);
        printf("CLIENT: Updated my stats - ELO: %d, Games: %d, Wins: %d, Losses: %d\n",
               game->my_elo, game->total_games, game->wins, game->losses);
        return 1;
    }
    
    else if(strncmp(msg, "LOGIN_FAIL", 10) == 0) {
        // Parse chi tiết message từ server: LOGIN_FAIL:reason
        const char* colon = strchr(msg, ':');
        if(colon && strlen(colon + 1) > 0) {
            // Có message chi tiết từ server
            char reason[256];
            strncpy(reason, colon + 1, sizeof(reason) - 1);
            reason[sizeof(reason) - 1] = '\0';

            // Xóa ký tự '#' nếu có ở cuối
            char* hash = strchr(reason, '#');
            if(hash) *hash = '\0';

            // Hiển thị message bằng tiếng Anh
            if(strstr(reason, "already logged in")) {
                strcpy(game->login_message, "Account is already logged in elsewhere!");
            } else if(strstr(reason, "Invalid credentials")) {
                strcpy(game->login_message, "Invalid username or password!");
            } else {
                // Hiển thị message gốc từ server
                snprintf(game->login_message, sizeof(game->login_message), "Login failed: %s", reason);
            }
        } else {
            // Không có message chi tiết, hiển thị message mặc định
            strcpy(game->login_message, "Invalid username or password!");
        }
        return 0;
    }
    return 0;
}

// ==================== PARSE USERS LIST ====================
int parse_users_list(GameData* game, const char* msg) {
    if(strncmp(msg, "USERS:", 6) != 0) return 0;
    
    char* ptr = (char*)(msg + 6);
    sscanf(ptr, "%d:", &game->user_count);
    ptr = strchr(ptr, ':') + 1;
    
    for(int i=0; i<game->user_count && i < 20; i++) {
        // Format: user_id,username,status,elo_rating,in_game
        sscanf(ptr, "%d,%[^,],%[^,],%d,%d", 
               &game->users[i].user_id,
               game->users[i].username,
               game->users[i].status,
               &game->users[i].elo_rating,
               &game->users[i].in_game);
        ptr = strchr(ptr + 1, ':');
        if(!ptr) break;
        ptr++;
    }
    
    return 1;
}

// ==================== PARSE SERVER MESSAGE ====================
int parse_server_message(GameData* game, const char* msg) {
    if(!msg) return 0;
    
    printf("Server: %s\n", msg);
    
    // STATE message
    if(strncmp(msg, "STATE:", 6) == 0) {
        return parse_state_message(game, msg);
    }
    
    // Login/Register
    if(strncmp(msg, "REGISTER_OK", 11) == 0) {
        strcpy(game->login_message, "Registration successful! Please log in.");
        game->is_register_mode = 0;
        return 1;
    }
    if(strncmp(msg, "REGISTER_FAIL", 13) == 0) {
        strcpy(game->login_message, "Registration failed! Username already exists.");
        return 1;
    }
    if(parse_login_response(game, msg)) {
        game->state = STATE_LOBBY;
        return 1;
    }
    
    // Users list
    if(parse_users_list(game, msg)) {
        return 1;
    }

    // Leaderboard
    if(strncmp(msg, "LEADERBOARD:", 12) == 0) {
        // Format: LEADERBOARD:rank:username:elo:games:wins:rank:username:elo:games:wins:...
        const char* data = msg + 12;
        game->leaderboard_count = 0;

        char temp[BUFF_SIZE];
        strncpy(temp, data, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        // Remove trailing '#' if present
        char* hash = strchr(temp, '#');
        if(hash) *hash = '\0';

        char* saveptr = NULL;
        char* token = strtok_r(temp, ":", &saveptr);

        while(token && game->leaderboard_count < 10) {
            LeaderboardEntry* entry = &game->leaderboard[game->leaderboard_count];

            // Parse rank:username:elo:games:wins
            entry->rank = atoi(token);

            token = strtok_r(NULL, ":", &saveptr);
            if(!token) break;
            strncpy(entry->username, token, sizeof(entry->username) - 1);

            token = strtok_r(NULL, ":", &saveptr);
            if(!token) break;
            entry->elo_rating = atoi(token);

            token = strtok_r(NULL, ":", &saveptr);
            if(!token) break;
            entry->total_games = atoi(token);

            token = strtok_r(NULL, ":", &saveptr);
            if(!token) break;
            entry->wins = atoi(token);

            game->leaderboard_count++;

            // Move to next entry
            token = strtok_r(NULL, ":", &saveptr);
        }

        printf("Leaderboard loaded: %d entries\n", game->leaderboard_count);
        return 1;
    }

    // Invite system
    if(strncmp(msg, "INVITE_SENT", 11) == 0) {
        // Transition from SENDING to WAITING
        game->state = STATE_WAITING_INVITE;
        strcpy(game->message, "Waiting for opponent's response...");
        printf("✓ CLIENT: Invite sent successfully, now waiting for response\n");
        return 1;
    }
    if(strncmp(msg, "INVITE_FAIL", 11) == 0) {
        // Failed to send invite - return to lobby with error message
        game->state = STATE_LOBBY;
        const char* reason = strchr(msg, ':');
        if(reason && strlen(reason + 1) > 0) {
            char error[256];
            strncpy(error, reason + 1, sizeof(error) - 1);
            error[sizeof(error) - 1] = '\0';
            char* hash = strchr(error, '#');
            if(hash) *hash = '\0';
            snprintf(game->message, sizeof(game->message), "Invite failed: %s", error);
        } else {
            strcpy(game->message, "Failed to send invitation!");
        }
        printf("✗ CLIENT: Invite failed - %s\n", game->message);
        return 1;
    }
    if(strncmp(msg, "INVITE_FROM:", 12) == 0) {
        sscanf(msg, "INVITE_FROM:%d:%[^#]",
               &game->inviter_user_id, game->inviter_username);
        game->state = STATE_RECEIVED_INVITE;
        // Save as last opponent for potential rematch
        strcpy(game->last_opponent_name, game->inviter_username);
        game->last_opponent_id = game->inviter_user_id;
        printf("✓ CLIENT: Received invite from user_id=%d, username=%s\n",
               game->inviter_user_id, game->inviter_username);
        return 1;
    }
    if(strncmp(msg, "INVITE_ACCEPTED", 15) == 0) {
        game->state = STATE_PLACING_SHIPS;
        // Save invited user as last opponent for rematch
        strcpy(game->last_opponent_name, game->invited_username);
        game->last_opponent_id = game->invited_user_id;
        strcpy(game->game_message, "Opponent accepted! Please place your ships.");
        return 1;
    }
    if(strncmp(msg, "INVITE_DECLINED", 15) == 0) {
        game->state = STATE_LOBBY;
        char username[50] = "";
        // Try to parse username from message
        if(sscanf(msg, "INVITE_DECLINED:%[^#]", username) == 1) {
            snprintf(game->message, sizeof(game->message), "%s declined the invitation!", username);
        } else {
            strcpy(game->message, "Opponent declined the invitation!");
        }
        return 1;
    }
    if(strncmp(msg, "DECLINE_OK", 10) == 0) {
        game->state = STATE_LOBBY;
        strcpy(game->message, "Invitation declined!");
        return 1;
    }
    if(strncmp(msg, "INVITE_CANCELLED", 16) == 0) {
        game->state = STATE_LOBBY;
        strcpy(game->message, "Invitation was cancelled!");
        return 1;
    }
    if(strncmp(msg, "INVITE_CANCEL_OK", 16) == 0) {
        game->state = STATE_LOBBY;
        strcpy(game->message, "Invitation cancelled!");
        return 1;
    }
    
    // Game messages
    if(strncmp(msg, "GAME_START", 10) == 0) {
        // Reset maps for new game
        for(int i=0; i<MAP_SIZE; i++) {
            for(int j=0; j<MAP_SIZE; j++) {
                game->own_map[i][j] = '-';
                game->enemy_map[i][j] = '-';
            }
        }
        // Reset ship placement state
        placing_ships_init(game);
        game->state = STATE_PLACING_SHIPS;
        return 1;
    }
    if(strncmp(msg, "START_PLAYING", 13) == 0) {
        game->state = STATE_PLAYING;
        game->is_my_turn = 0;
        game->game_start_time = SDL_GetTicks();  // Start timer
        game->elo_predicted = game->my_elo;  // Initialize prediction
        snprintf(game->message, sizeof(game->message), "Game started! Good luck!");
        return 1;
    }
    if(strncmp(msg, "WAITING_OPPONENT", 16) == 0) {
        snprintf(game->message, sizeof(game->message), "Waiting for opponent to READY...");
        return 1;
    }
    if(strncmp(msg, "OPPONENT_READY", 14) == 0) {
        snprintf(game->message, sizeof(game->message), "Opponent is READY! Click READY when done.");
        return 1;
    }
    if(strcmp(msg, "YOUR_TURN:") == 0) {
        game->is_my_turn = 1;
        game->state = STATE_PLAYING;
        snprintf(game->message, sizeof(game->message), "Your turn!");
        return 1;
    }
    if(strcmp(msg, "WAIT_YOUR_TURN:") == 0) {
        game->is_my_turn = 0;
        game->state = STATE_PLAYING;
        snprintf(game->message, sizeof(game->message), "Opponent is firing...");
        return 1;
    }
    if(strcmp(msg, "HIT_CONTINUE:") == 0) {
        snprintf(game->message, sizeof(game->message), "HIT! Fire again!");
        return 1;
    }
    
    // Fire result (HIT or MISS)
    if(strncmp(msg, "RESULT:HIT,", 11) == 0) {
        game->hits_count++;
        game->total_shots++;
        // Message already set by playing screen
        return 1;
    }
    if(strncmp(msg, "RESULT:MISS,", 12) == 0) {
        game->misses_count++;
        game->total_shots++;
        // Message already set by playing screen
        return 1;
    }
    
    if(strncmp(msg, "GAME_OVER:WIN:", 14) == 0) {
        // Parse: GAME_OVER:WIN:Opponent surrendered:1564:+25#
        char reason[128];
        int new_elo = 0;
        int elo_change = 0;
        sscanf(msg + 14, "%[^:]:%d:%d", reason, &new_elo, &elo_change);
        
        // Calculate game duration
        unsigned int elapsed_ms = SDL_GetTicks() - game->game_start_time;
        game->game_duration_seconds = elapsed_ms / 1000;
        
        game->state = STATE_GAME_OVER;
        game->game_result_won = 1;
        game->elo_before = game->my_elo;
        game->my_elo = new_elo;
        game->elo_change = elo_change;
        game->elo_bonus = 0;
        
        snprintf(game->message, sizeof(game->message), "YOU WIN! %s ELO: %+d", reason, elo_change);
        printf("CLIENT: Won game (%s), ELO change: %+d, new ELO: %d\n", reason, elo_change, new_elo);
        return 1;
    }
    if(strncmp(msg, "YOU WIN:", 8) == 0) {
        // Parse: YOU WIN:ELO +25#
        int elo_change = 0;
        sscanf(msg, "YOU WIN:ELO %d", &elo_change);
        
        // Calculate game duration
        unsigned int elapsed_ms = SDL_GetTicks() - game->game_start_time;
        game->game_duration_seconds = elapsed_ms / 1000;
        
        game->state = STATE_GAME_OVER;
        game->game_result_won = 1;
        game->elo_before = game->my_elo;
        game->elo_change = elo_change;
        game->elo_bonus = 0;  // Server will send total, we'll parse bonus later
        
        // Update my_elo
        game->my_elo += elo_change;
        
        snprintf(game->message, sizeof(game->message), "YOU WIN! ELO: %+d", elo_change);
        printf("CLIENT: Won game, ELO change: %+d, new ELO: %d\n", elo_change, game->my_elo);
        return 1;
    }
    if(strncmp(msg, "GAME_OVER:LOSE:", 15) == 0) {
        // Parse: GAME_OVER:LOSE:You surrendered:1539:-25#
        char reason[128];
        int new_elo = 0;
        int elo_change = 0;
        sscanf(msg + 15, "%[^:]:%d:%d", reason, &new_elo, &elo_change);
        
        // Calculate game duration
        unsigned int elapsed_ms = SDL_GetTicks() - game->game_start_time;
        game->game_duration_seconds = elapsed_ms / 1000;
        
        game->state = STATE_GAME_OVER;
        game->game_result_won = 0;
        game->elo_before = game->my_elo;
        game->my_elo = new_elo;
        game->elo_change = elo_change;
        game->elo_bonus = 0;
        
        snprintf(game->message, sizeof(game->message), "YOU LOSE! %s ELO: %+d", reason, elo_change);
        printf("CLIENT: Lost game (%s), ELO change: %+d, new ELO: %d\n", reason, elo_change, new_elo);
        return 1;
    }
    if(strncmp(msg, "YOU LOSE:", 9) == 0) {
        // Parse: YOU LOSE:ELO -25#
        int elo_change = 0;
        sscanf(msg, "YOU LOSE:ELO %d", &elo_change);
        
        // Calculate game duration
        unsigned int elapsed_ms = SDL_GetTicks() - game->game_start_time;
        game->game_duration_seconds = elapsed_ms / 1000;
        
        game->state = STATE_GAME_OVER;
        game->game_result_won = 0;
        game->elo_before = game->my_elo;
        game->elo_change = elo_change;
        game->elo_bonus = 0;
        
        // Update my_elo
        game->my_elo += elo_change;
        
        snprintf(game->message, sizeof(game->message), "YOU LOSE! ELO: %+d", elo_change);
        printf("CLIENT: Lost game, ELO change: %+d, new ELO: %d\n", elo_change, game->my_elo);
        return 1;
    }
    if(strncmp(msg, "MATCH_START:", 12) == 0) {
        game->state = STATE_PLAYING;
        game->elo_predicted = game->my_elo;  // Reset prediction
        
        // Reset game statistics
        game->total_shots = 0;
        game->hits_count = 0;
        game->misses_count = 0;
        
        snprintf(game->message, sizeof(game->message), "Match started!");
        return 1;
    }
    if(strncmp(msg, "PLACE_OK:", 9) == 0) {
        // Increment counter now that server confirmed placement
        if(game->last_placed_ship_length > 0) {
            game->ships_placed_count[game->last_placed_ship_length]++;
            int max = (game->last_placed_ship_length == 2) ? 2 : 1;
            snprintf(game->message, sizeof(game->message), "Da dat tau %d o! (%d/%d)",
                     game->last_placed_ship_length,
                     game->ships_placed_count[game->last_placed_ship_length], max);
            game->last_placed_ship_length = 0;  // Reset
        } else {
            snprintf(game->message, sizeof(game->message), "Ship placed successfully!");
        }
        return 1;
    }
    if(strcmp(msg, "READY_OK:") == 0) {
        snprintf(game->message, sizeof(game->message), "READY! Waiting for opponent...");
        return 1;
    }

    // Logout
    if(strncmp(msg, "LOGOUT_OK", 9) == 0) {
        // Clear session data
        game->state = STATE_LOGIN;
        game->my_user_id = -1;
        game->my_username[0] = '\0';
        game->my_elo = 0;
        game->total_games = 0;
        game->wins = 0;
        game->losses = 0;

        // Clear input fields
        game->username_field.text[0] = '\0';
        game->password_field.text[0] = '\0';

        strcpy(game->login_message, "Logged out successfully. Please login again.");
        return 1;
    }

    // Forfeit messages
    if(strncmp(msg, "FORFEIT_PLACEMENT", 17) == 0) {
        // Reset maps
        for(int i=0; i<MAP_SIZE; i++) {
            for(int j=0; j<MAP_SIZE; j++) {
                game->own_map[i][j] = '-';
                game->enemy_map[i][j] = '-';
            }
        }
        
        game->state = STATE_LOBBY;
        strcpy(game->message, "You left during ship placement");
        printf("CLIENT: Forfeited during placement, returned to lobby\n");
        return 1;
    }
    if(strncmp(msg, "OPPONENT_LEFT_PLACEMENT", 23) == 0) {
        // Extract opponent name if provided
        char* colon = strchr(msg, ':');
        if(colon) {
            sscanf(colon + 1, "%[^#]", game->opponent_quit_name);
        } else {
            strcpy(game->opponent_quit_name, "Opponent");
        }
        
        game->state = STATE_OPPONENT_QUIT_PLACEMENT;
        printf("CLIENT: Opponent quit during placement\n");
        return 1;
    }
    // GAME_OVER messages already handle forfeit case (WIN/LOSE with reason)

    // Surrender messages
    if(strncmp(msg, "SURRENDER_REQUEST_FROM:", 23) == 0) {
        // Format: SURRENDER_REQUEST_FROM:username#
        sscanf(msg + 23, "%[^#]", game->surrender_requester_name);
        // Hide any confirmation dialog to prevent blocking surrender buttons
        game->confirmation_dialog.visible = 0;
        game->state = STATE_RECEIVED_SURRENDER_REQUEST;
        printf("CLIENT: Received surrender request from %s\n", game->surrender_requester_name);
        return 1;
    }
    if(strncmp(msg, "SURRENDER_DECLINED", 18) == 0) {
        game->state = STATE_PLAYING;
        strcpy(game->message, "Opponent declined your surrender request");
        printf("CLIENT: Surrender request declined\n");
        return 1;
    }
    // SURRENDER_ACCEPTED will trigger GAME_OVER message from server

    // Rematch messages
    if(strncmp(msg, "REMATCH_REQUEST_FROM:", 21) == 0) {
        // Format: REMATCH_REQUEST_FROM:username#
        sscanf(msg + 21, "%[^#]", game->rematch_requester_name);
        game->state = STATE_RECEIVED_REMATCH_REQUEST;
        printf("CLIENT: Received rematch request from %s\n", game->rematch_requester_name);
        return 1;
    }
    if(strncmp(msg, "REMATCH_DECLINED", 16) == 0) {
        game->state = STATE_LOBBY;
        strcpy(game->message, "Opponent declined your rematch request");
        printf("CLIENT: Rematch request declined\n");
        return 1;
    }
    if(strncmp(msg, "BOTH_WANT_REMATCH", 17) == 0) {
        // Both players clicked rematch simultaneously, go directly to placing ships
        printf("CLIENT: Both players want rematch, starting new game\n");
        // Server will send GAME_START next
        return 1;
    }
    if(strncmp(msg, "WAITING_REMATCH_RESPONSE", 24) == 0) {
        // You requested rematch, opponent hasn't responded yet
        game->state = STATE_WAITING_REMATCH_RESPONSE;
        printf("CLIENT: Waiting for opponent's rematch response\n");
        return 1;
    }

    // Matchmaking
    if(strncmp(msg, "MM_JOINED", 9) == 0) {
        printf("CLIENT: Joined matchmaking queue\n");
        return 1;
    }
    if(strncmp(msg, "MM_CANCELLED", 12) == 0) {
        game->state = STATE_LOBBY;
        game->matchmaking_active = 0;
        printf("CLIENT: Matchmaking cancelled\n");
        return 1;
    }
    if(strncmp(msg, "MATCH_FOUND:", 12) == 0) {
        // Format: MATCH_FOUND:opponent_name:opponent_id:opponent_elo
        char opponent_name[50];
        int opponent_id = 0;
        int opponent_elo = 0;
        
        sscanf(msg + 12, "%[^:]:%d:%d", opponent_name, &opponent_id, &opponent_elo);
        
        strcpy(game->matched_opponent_name, opponent_name);
        game->matched_opponent_id = opponent_id;
        game->matched_opponent_elo = opponent_elo;
        game->matchmaking_active = 0;
        
        // Transition to match found screen (wait for accept/decline)
        game->state = STATE_MATCH_FOUND;
        
        snprintf(game->message, sizeof(game->message), 
                "Match found! vs %s (ID:%d, ELO:%d)", opponent_name, opponent_id, opponent_elo);
        printf("CLIENT: Match found! Opponent: %s (ID:%d, ELO:%d)\n", 
               opponent_name, opponent_id, opponent_elo);
        return 1;
    }
    if(strncmp(msg, "MM_ERROR", 8) == 0) {
        game->state = STATE_LOBBY;
        game->matchmaking_active = 0;
        strcpy(game->message, "Matchmaking error!");
        printf("CLIENT: Matchmaking error\n");
        return 1;
    }
    if(strncmp(msg, "MATCH_ACCEPTED:", 15) == 0) {
        if(strstr(msg, "GAME_START") != NULL) {
            // Both players accepted - start game
            // Reset maps for new game
            for(int i=0; i<MAP_SIZE; i++) {
                for(int j=0; j<MAP_SIZE; j++) {
                    game->own_map[i][j] = '-';
                    game->enemy_map[i][j] = '-';
                }
            }
            // Reset ship placement state
            placing_ships_init(game);
            game->state = STATE_PLACING_SHIPS;
            strcpy(game->message, "Match accepted! Place your ships.");
            printf("CLIENT: Both players accepted - starting game\n");
        } else if(strstr(msg, "WAITING_OPPONENT") != NULL) {
            // Waiting for opponent to accept
            strcpy(game->message, "Waiting for opponent to accept...");
            printf("CLIENT: Waiting for opponent to accept\n");
        }
        return 1;
    }
    if(strncmp(msg, "OPPONENT_ACCEPTED", 17) == 0) {
        strcpy(game->message, "Opponent accepted! Waiting for you...");
        printf("CLIENT: Opponent accepted the match\n");
        return 1;
    }
    if(strncmp(msg, "MATCH_DECLINED:", 15) == 0) {
        if(strstr(msg, "OK") != NULL) {
            // You declined
            game->state = STATE_LOBBY;
            strcpy(game->message, "Match declined.");
            printf("CLIENT: You declined the match\n");
        } else {
            // Opponent declined
            char opponent_name[50];
            sscanf(msg + 15, "%[^#]", opponent_name);
            game->state = STATE_LOBBY;
            snprintf(game->message, sizeof(game->message), "%s declined the match.", opponent_name);
            printf("CLIENT: Opponent %s declined\n", opponent_name);
        }
        return 1;
    }

    // Match history
    if(strncmp(msg, "MATCH_HISTORY:", 14) == 0) {
        // Format: MATCH_HISTORY:count|match1_data|match2_data|...
        // match_data: match_id,timestamp,opponent_id,opponent_name,is_win,hits,misses,elo_change,duration
        
        printf("DEBUG: Parsing MATCH_HISTORY message: %s\n", msg);
        
        const char* data = msg + 14;
        int count = 0;
        sscanf(data, "%d", &count);
        
        printf("DEBUG: Match count from server: %d\n", count);
        
        game->match_history_count = 0;
        
        // Skip count and first '|'
        const char* ptr = strchr(data, '|');
        if(!ptr) {
            printf("DEBUG: No pipe separator found, empty history\n");
            return 1;
        }
        ptr++; // Skip '|'
        
        char temp[BUFF_SIZE * 2];
        strncpy(temp, ptr, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
        
        printf("DEBUG: Match data to parse: %s\n", temp);
        
        // Parse each match entry separated by '|'
        char* saveptr = NULL;
        char* token = strtok_r(temp, "|", &saveptr);
        
        while(token && game->match_history_count < 10) {
            MatchHistoryEntry* entry = &game->match_history[game->match_history_count];
            
            printf("DEBUG: Parsing token: %s\n", token);
            
            // Parse: match_id,timestamp,opponent_id,opponent_name,is_win,hits,misses,elo_change,duration
            long timestamp;
            int is_win, hits, misses;
            int parsed = sscanf(token, "%d,%ld,%d,%49[^,],%d,%d,%d,%d,%d",
                               &entry->match_id, &timestamp, 
                               &entry->opponent_id, entry->opponent_name,
                               &is_win, &hits, &misses,
                               &entry->elo_change, &entry->duration_seconds);
            
            printf("DEBUG: Parsed %d fields\n", parsed);
            
            if(parsed == 9) {
                entry->result = is_win;
                entry->my_hits = hits;
                entry->my_misses = misses;
                
                // Convert timestamp to date string
                time_t t = (time_t)timestamp;
                struct tm* tm_info = localtime(&t);
                strftime(entry->date, sizeof(entry->date), "%Y-%m-%d %H:%M", tm_info);
                
                printf("DEBUG: Successfully parsed match %d - %s vs %s\n", 
                       entry->match_id, "You", entry->opponent_name);
                
                game->match_history_count++;
            }
            
            token = strtok_r(NULL, "|", &saveptr);
        }
        
        printf("CLIENT: Match history loaded: %d entries\n", game->match_history_count);
        return 1;
    }

    // Match detail
    if(strncmp(msg, "MATCH_DETAIL:", 13) == 0) {
        // Format: MATCH_DETAIL:match_id:winner:my_name:opponent_name:my_ships:opponent_ships:match_data#
        // match_data format: P1_SHOTS|P2_SHOTS
        // Each shot: x,y,hit,ship_len,sunk;
        
        int match_id = 0;
        int winner = 0;
        char my_name[50] = {0};
        char opponent_name[50] = {0};
        char my_ships[MAP_SIZE * MAP_SIZE + 1] = {0};
        char opponent_ships[MAP_SIZE * MAP_SIZE + 1] = {0};
        char match_data[4096] = {0};
        
        // Parse: match_id:winner:my_name:opponent_name:my_ships:opponent_ships:match_data
        const char* ptr = msg + 13;
        sscanf(ptr, "%d:%d:", &match_id, &winner);
        
        // Skip match_id and winner
        ptr = strchr(ptr, ':'); if(ptr) ptr++;
        ptr = strchr(ptr, ':'); if(ptr) ptr++;
        
        // Extract my_name
        const char* next_colon = strchr(ptr, ':');
        if(next_colon) {
            int len = next_colon - ptr;
            if(len > 0 && len < 50) {
                strncpy(my_name, ptr, len);
                my_name[len] = '\0';
            }
            ptr = next_colon + 1;
        }
        
        // Extract opponent_name
        next_colon = strchr(ptr, ':');
        if(next_colon) {
            int len = next_colon - ptr;
            if(len > 0 && len < 50) {
                strncpy(opponent_name, ptr, len);
                opponent_name[len] = '\0';
            }
            ptr = next_colon + 1;
        }
        
        // Extract my_ships
        next_colon = strchr(ptr, ':');
        if(next_colon) {
            int len = next_colon - ptr;
            if(len > 0 && len < MAP_SIZE * MAP_SIZE + 1) {
                strncpy(my_ships, ptr, len);
                my_ships[len] = '\0';
            }
            ptr = next_colon + 1;
        }
        
        // Extract opponent_ships
        next_colon = strchr(ptr, ':');
        if(next_colon) {
            int len = next_colon - ptr;
            if(len > 0 && len < MAP_SIZE * MAP_SIZE + 1) {
                strncpy(opponent_ships, ptr, len);
                opponent_ships[len] = '\0';
            }
            ptr = next_colon + 1;
        }
        
        // Extract match_data (rest until #)
        const char* end = strchr(ptr, '#');
        if(end) {
            int len = end - ptr;
            if(len > 0 && len < 4096) {
                strncpy(match_data, ptr, len);
                match_data[len] = '\0';
            }
        }
        
        printf("CLIENT: Parsing MATCH_DETAIL - my_user_id=%d, match_data length=%zu\n", 
               game->my_user_id, strlen(match_data));
        printf("CLIENT: First 100 chars: %.100s\n", match_data);
        
        // Store in game data
        game->current_match_detail.match_id = match_id;
        game->current_match_detail.winner = winner;
        strncpy(game->current_match_detail.my_name, my_name, sizeof(game->current_match_detail.my_name) - 1);
        strncpy(game->current_match_detail.opponent_name, opponent_name, sizeof(game->current_match_detail.opponent_name) - 1);
        strncpy(game->current_match_detail.my_ships, my_ships, sizeof(game->current_match_detail.my_ships) - 1);
        strncpy(game->current_match_detail.opponent_ships, opponent_ships, sizeof(game->current_match_detail.opponent_ships) - 1);
        game->current_match_detail.my_shot_count = 0;
        game->current_match_detail.opponent_shot_count = 0;
        game->current_match_detail.shot_count = 0;  // Reset chronological shot count
        
        // Parse match_data: player_id:x,y,hit,ship_len,sunk;player_id:x,y,hit,ship_len,sunk;...
        if(strlen(match_data) > 0) {
            char temp[4096];
            strncpy(temp, match_data, sizeof(temp) - 1);
            temp[sizeof(temp) - 1] = '\0';
            
            // Parse shots: each shot has format "player_id:x,y,hit,ship_len,sunk;"
            char* saveptr = NULL;
            char* token = strtok_r(temp, ";", &saveptr);
            int token_count = 0;
            while(token && game->current_match_detail.shot_count < 200) {
                token_count++;
                // Check if this is a special marker (FORFEIT, SURRENDER)
                if(strcmp(token, "FORFEIT") == 0 || strcmp(token, "SURRENDER") == 0) {
                    printf("CLIENT: Skipping marker: %s\n", token);
                    token = strtok_r(NULL, ";", &saveptr);
                    continue;
                }
                
                int player_id = 0;
                int x, y, hit, ship_len, sunk;
                
                // Parse: player_id:x,y,hit,ship_len,sunk
                int parsed = sscanf(token, "%d:%d,%d,%d,%d,%d", &player_id, &x, &y, &hit, &ship_len, &sunk);
                if(parsed == 6) {
                    // Determine which player this shot belongs to
                    int is_my_shot = (player_id == game->my_user_id);
                    
                    printf("CLIENT: Token #%d: '%s' -> P%d at (%d,%d) hit=%d, is_mine=%d (my_id=%d)\n", 
                           token_count, token, player_id, x, y, hit, is_my_shot, game->my_user_id);
                    
                    // Add to chronological list
                    ShotEntry* shot = &game->current_match_detail.all_shots[game->current_match_detail.shot_count];
                    shot->x = x;
                    shot->y = y;
                    shot->hit = hit;
                    shot->ship_length = ship_len;
                    shot->ship_sunk = sunk;
                    shot->is_my_shot = is_my_shot;
                    game->current_match_detail.shot_count++;
                    
                    // Also add to old arrays for backward compatibility
                    if(is_my_shot && game->current_match_detail.my_shot_count < 100) {
                        ShotEntry* my_shot = &game->current_match_detail.my_shots[game->current_match_detail.my_shot_count];
                        my_shot->x = x;
                        my_shot->y = y;
                        my_shot->hit = hit;
                        my_shot->ship_length = ship_len;
                        my_shot->ship_sunk = sunk;
                        my_shot->is_my_shot = 1;
                        game->current_match_detail.my_shot_count++;
                    } else if(!is_my_shot && game->current_match_detail.opponent_shot_count < 100) {
                        ShotEntry* opp_shot = &game->current_match_detail.opponent_shots[game->current_match_detail.opponent_shot_count];
                        opp_shot->x = x;
                        opp_shot->y = y;
                        opp_shot->hit = hit;
                        opp_shot->ship_length = ship_len;
                        opp_shot->ship_sunk = sunk;
                        opp_shot->is_my_shot = 0;
                        game->current_match_detail.opponent_shot_count++;
                    }
                } else {
                    printf("CLIENT: Failed to parse token #%d: '%s' (parsed %d fields)\n", 
                           token_count, token, parsed);
                }
                
                token = strtok_r(NULL, ";", &saveptr);
            }
            
            printf("CLIENT: Parsed %d tokens, final shot_count=%d\n", token_count, game->current_match_detail.shot_count);
        }
        
        printf("CLIENT: Match detail loaded - Match #%d, Winner=%d, Total shots=%d (My=%d, Opp=%d)\n",
               match_id, winner, game->current_match_detail.shot_count,
               game->current_match_detail.my_shot_count, game->current_match_detail.opponent_shot_count);
        
        // Transition to detail screen
        game->state = STATE_MATCH_DETAIL;
        return 1;
    }
    
    // AFK warning
    if(strcmp(msg, "AFK_WARNING#") == 0) {
        game->afk_warning_visible = 1;
        printf("CLIENT: AFK warning received\n");
        return 1;
    }

    return 0;
}
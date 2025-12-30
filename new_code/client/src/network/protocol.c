// protocol.c - Protocol parsing implementation
#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    
    // Count ships from map
    for(int i = 2; i <= 4; i++) game->ships_placed_count[i] = 0;
    
    int visited[MAP_SIZE][MAP_SIZE] = {0};
    for(int r = 0; r < MAP_SIZE; r++) {
        for(int c = 0; c < MAP_SIZE; c++) {
            char ch = game->own_map[r][c];
            if(ch >= '2' && ch <= '9' && !visited[r][c]) {
                int length = ch - '0';
                
                // Mark all cells of this ship
                int ship_len = 0;
                for(int cc = c; cc < MAP_SIZE && game->own_map[r][cc] == ch; cc++) {
                    visited[r][cc] = 1;
                    ship_len++;
                }
                
                if(ship_len == 0) {
                    for(int rr = r; rr < MAP_SIZE && game->own_map[rr][c] == ch; rr++) {
                        visited[rr][c] = 1;
                        ship_len++;
                    }
                }
                
                if(length >= 2 && length <= 4) {
                    game->ships_placed_count[length]++;
                }
            }
        }
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
    } else if(strncmp(msg, "LOGIN_FAIL", 10) == 0) {
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
        sscanf(ptr, "%d,%[^,],%[^,],%d", 
               &game->users[i].user_id,
               game->users[i].username,
               game->users[i].status,
               &game->users[i].elo_rating);
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
        printf("✓ CLIENT: Received invite from user_id=%d, username=%s\n",
               game->inviter_user_id, game->inviter_username);
        return 1;
    }
    if(strncmp(msg, "INVITE_ACCEPTED", 15) == 0) {
        game->state = STATE_PLACING_SHIPS;
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
        game->state = STATE_PLACING_SHIPS;
        return 1;
    }
    if(strncmp(msg, "START_PLAYING", 13) == 0) {
        game->state = STATE_PLAYING;
        game->is_my_turn = 0;
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
    if(strncmp(msg, "YOU WIN:", 8) == 0) {
        game->state = STATE_GAME_OVER;
        snprintf(game->message, sizeof(game->message), "YOU WIN!");
        return 1;
    }
    if(strncmp(msg, "YOU LOSE:", 9) == 0) {
        game->state = STATE_GAME_OVER;
        snprintf(game->message, sizeof(game->message), "YOU LOSE!");
        return 1;
    }
    if(strncmp(msg, "MATCH_START:", 12) == 0) {
        game->state = STATE_PLAYING;
        snprintf(game->message, sizeof(game->message), "Match started!");
        return 1;
    }
    if(strncmp(msg, "PLACE_OK:", 9) == 0) {
        snprintf(game->message, sizeof(game->message), "Ship placed successfully!");
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

    return 0;
}


// server_lobby.c - Server with Lobby system and invite mechanism
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "database.h"

#define PORT 5501
#define MAX_CLIENTS 50
#define BUFF_SIZE 8192
#define MAP_SIZE 13
#define MAX_SHIP 4

// Ship structure
typedef struct {
    int length;
    int x[MAP_SIZE];
    int y[MAP_SIZE];
    int hits;
    int alive;
} Ship;

// Client structure (extended with game data)
typedef struct {
    int fd;
    int user_id;
    char username[50];
    int is_authenticated;
    int in_game;
    int invited_by;  // user_id của người mời
    int opponent_id; // user_id của đối thủ
    int is_ready;    // đã sẵn sàng chơi chưa
    pthread_t thread;

    // Game data
    char map[MAP_SIZE][MAP_SIZE];        // Bản đồ của mình
    char enemy_map[MAP_SIZE][MAP_SIZE];  // Bản đồ địch (để tracking)
    Ship ships[MAX_SHIP];                // Danh sách tàu
    int ship_count;                      // Số tàu đã đặt
    int is_my_turn;                      // Lượt của mình không
    
    // Game statistics (for ELO calculation)
    int total_shots;                     // Tổng số lần bắn
    int total_hits;                      // Số lần bắn trúng
    time_t game_start_time;              // Thời điểm bắt đầu game
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

// ==================== GAME HELPER FUNCTIONS ====================

// Khởi tạo bản đồ rỗng
void init_map(char map[MAP_SIZE][MAP_SIZE]) {
    for(int i=0; i<MAP_SIZE; i++)
        for(int j=0; j<MAP_SIZE; j++)
            map[i][j] = '-';
}

// Kiểm tra người chơi đã đặt đủ tàu chưa (1x4, 1x3, 2x2)
int check_ready_ships(Client *client) {
    int count4=0, count3=0, count2=0;
    for(int i=0; i<client->ship_count; i++) {
        if(client->ships[i].length == 4) count4++;
        else if(client->ships[i].length == 3) count3++;
        else if(client->ships[i].length == 2) count2++;
    }
    return (count4 >= 1 && count3 >= 1 && count2 >= 2);
}

// Đánh dấu tàu đã chìm
void mark_sunk(Client *client, Client *enemy) {
    for(int s=0; s<client->ship_count; s++) {
        Ship *sh = &client->ships[s];
        if(sh->alive == 0) {
            for(int k=0; k<sh->length; k++) {
                client->map[sh->y[k]][sh->x[k]] = '@';
                enemy->enemy_map[sh->y[k]][sh->x[k]] = '@';
            }
        }
    }
}

// Gửi trạng thái bản đồ
void send_state(Client *client) {
    char msg[BUFF_SIZE];
    msg[0] = '\0';

    strcat(msg, "STATE:\nOWN MAP\n");
    for(int i=0; i<MAP_SIZE; i++) {
        for(int j=0; j<MAP_SIZE; j++) {
            char tmp[2];
            tmp[0] = client->map[i][j];
            tmp[1] = '\0';
            strcat(msg, tmp);
            strcat(msg, " ");
        }
        strcat(msg, "\n");
    }

    strcat(msg, "ENEMY MAP\n");
    for(int i=0; i<MAP_SIZE; i++) {
        for(int j=0; j<MAP_SIZE; j++) {
            char tmp[2];
            tmp[0] = client->enemy_map[i][j];
            tmp[1] = '\0';
            strcat(msg, tmp);
            strcat(msg, " ");
        }
        strcat(msg, "\n");
    }

    strcat(msg, "#");
    send(client->fd, msg, strlen(msg), 0);
}

// ==================== FIND CLIENT BY USER_ID ====================
Client* find_client_by_user_id(int user_id) {
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].fd > 0 && clients[i].user_id == user_id) {
            return &clients[i];
        }
    }
    return NULL;
}

// ==================== FIND CLIENT BY FD ====================
Client* find_client_by_fd(int fd) {
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].fd == fd) {
            return &clients[i];
        }
    }
    return NULL;
}

// ==================== SEND TO CLIENT ====================
void send_to_client(int fd, const char* msg) {
    send(fd, msg, strlen(msg), 0);
    printf("SENT to fd=%d: %s\n", fd, msg);
}

// ==================== HANDLE REGISTER ====================
void handle_register(Client* client, char* buffer) {
    char username[50], password[50];
    // Remove trailing '#' if present
    char* hash = strchr(buffer, '#');
    if(hash) *hash = '\0';
    
    if(sscanf(buffer+9, "%[^:]:%s", username, password) == 2) {
        printf("DEBUG: Register username='%s', password='%s'\n", username, password);
        int user_id = db_register_user(username, password);
        if(user_id > 0) {
            send_to_client(client->fd, "REGISTER_OK#");
            printf("User registered: %s (ID: %d)\n", username, user_id);
        } else {
            send_to_client(client->fd, "REGISTER_FAIL:Username exists#");
        }
    } else {
        send_to_client(client->fd, "REGISTER_FAIL:Invalid format#");
    }
}

// ==================== HANDLE LOGIN ====================
void handle_login(Client* client, char* buffer) {
    char username[50], password[50];
    // Remove trailing '#' if present
    char* hash = strchr(buffer, '#');
    if(hash) *hash = '\0';

    if(sscanf(buffer+6, "%[^:]:%s", username, password) == 2) {
        printf("DEBUG: Parsed username='%s', password='%s'\n", username, password);
        UserProfile profile;
        int user_id = db_login_user(username, password, &profile);

        if(user_id > 0) {
            pthread_mutex_lock(&clients_lock);

            // Check if user is already logged in
            for(int i=0; i<MAX_CLIENTS; i++) {
                if(clients[i].fd > 0 && clients[i].user_id == user_id && clients[i].fd != client->fd) {
                    pthread_mutex_unlock(&clients_lock);
                    send_to_client(client->fd, "LOGIN_FAIL:User already logged in#");
                    printf("Login denied: %s already logged in\n", username);
                    return;
                }
            }

            client->user_id = user_id;
            strcpy(client->username, profile.username);
            client->is_authenticated = 1;
            pthread_mutex_unlock(&clients_lock);

            char response[BUFF_SIZE];
            snprintf(response, sizeof(response),
                    "LOGIN_OK:%s:%d:%d:%d:%d#",
                    profile.username, profile.total_games,
                    profile.wins, profile.elo_rating, user_id);
            send_to_client(client->fd, response);
            printf("User logged in: %s (ELO: %d)\n", username, profile.elo_rating);
        } else {
            send_to_client(client->fd, "LOGIN_FAIL:Invalid credentials#");
        }
    }
}

// ==================== HANDLE GET_MY_STATS ====================
void handle_get_my_stats(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    UserProfile profile;
    if(db_get_user_profile(client->user_id, &profile) == 0) {
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response),
                "MY_STATS:%s:%d:%d:%d:%d#",
                profile.username, profile.total_games,
                profile.wins, profile.elo_rating, client->user_id);
        send_to_client(client->fd, response);
        printf("Sent stats to %s: ELO=%d, Games=%d, Wins=%d\n",
               profile.username, profile.elo_rating, profile.total_games, profile.wins);
    } else {
        send_to_client(client->fd, "ERROR:Failed to get stats#");
    }
}

// ==================== HANDLE GET_USERS ====================
void handle_get_users(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    char response[BUFF_SIZE];
    int offset = sprintf(response, "USERS:");
    
    // Count online users
    pthread_mutex_lock(&clients_lock);
    int count = 0;
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].fd > 0 && clients[i].is_authenticated) {
            count++;
        }
    }
    
    offset += sprintf(response + offset, "%d:", count);
    
    // Add user info
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].fd > 0 && clients[i].is_authenticated) {
            UserProfile profile;
            if(db_get_user_profile(clients[i].user_id, &profile) == 0) {
                offset += snprintf(response + offset, BUFF_SIZE - offset,
                    "%d,%s,%s,%d:",
                    profile.user_id,
                    profile.username,
                    profile.status,
                    profile.elo_rating
                );
            }
        }
    }
    pthread_mutex_unlock(&clients_lock);
    
    strcat(response, "#");
    send_to_client(client->fd, response);
}

// ==================== HANDLE INVITE ====================
void handle_invite(Client* client, char* buffer) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }

    int target_user_id;
    sscanf(buffer+7, "%d", &target_user_id);

    pthread_mutex_lock(&clients_lock);

    // Debug: Print all connected clients
    printf("\n=== DEBUG: All connected clients ===\n");
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].fd > 0) {
            printf("  [%d] fd=%d, user_id=%d, username='%s', auth=%d, in_game=%d\n",
                   i, clients[i].fd, clients[i].user_id, clients[i].username,
                   clients[i].is_authenticated, clients[i].in_game);
        }
    }
    printf("=== Looking for user_id=%d ===\n\n", target_user_id);

    Client* target = find_client_by_user_id(target_user_id);

    // Debug logging
    if(!target) {
        printf("INVITE DEBUG: User %d not found in clients list\n", target_user_id);
        send_to_client(client->fd, "INVITE_FAIL:User not available#");
    } else if(!target->is_authenticated) {
        printf("INVITE DEBUG: User %d (fd=%d) not authenticated\n", target_user_id, target->fd);
        send_to_client(client->fd, "INVITE_FAIL:User not available#");
    } else if(target->in_game) {
        printf("INVITE DEBUG: User %d (%s) already in game\n", target_user_id, target->username);
        send_to_client(client->fd, "INVITE_FAIL:User not available#");
    } else {
        // All checks passed - send invite
        char msg[256];
        snprintf(msg, sizeof(msg), "INVITE_FROM:%d:%s#",
                 client->user_id, client->username);
        send_to_client(target->fd, msg);

        // Store who invited whom
        target->invited_by = client->user_id;
        client->opponent_id = target_user_id;

        send_to_client(client->fd, "INVITE_SENT#");
        printf("✓ Invite sent from %s (id=%d) to %s (id=%d)\n",
               client->username, client->user_id, target->username, target_user_id);
    }
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE ACCEPT_INVITE ====================
void handle_accept_invite(Client* client, char* buffer) {
    int inviter_id;
    sscanf(buffer+14, "%d", &inviter_id);
    
    pthread_mutex_lock(&clients_lock);
    Client* inviter = find_client_by_user_id(inviter_id);
    
    if(inviter && inviter->is_authenticated) {
        // Notify both clients
        send_to_client(inviter->fd, "INVITE_ACCEPTED#");
        send_to_client(client->fd, "INVITE_ACCEPTED#");
        
        // Set up game pairing
        client->in_game = 1;
        inviter->in_game = 1;
        client->invited_by = inviter_id;  // Mark who invited (for turn order)
        client->opponent_id = inviter_id;
        inviter->opponent_id = client->user_id;
        client->is_ready = 0;
        inviter->is_ready = 0;

        // Initialize game data
        init_map(client->map);
        init_map(client->enemy_map);
        init_map(inviter->map);
        init_map(inviter->enemy_map);
        client->ship_count = 0;
        inviter->ship_count = 0;
        client->is_my_turn = 0;
        inviter->is_my_turn = 0;
        
        // Initialize game statistics
        client->total_shots = 0;
        client->total_hits = 0;
        inviter->total_shots = 0;
        inviter->total_hits = 0;
        client->game_start_time = 0;  // Will be set when game actually starts
        inviter->game_start_time = 0;

        // Send GAME_START to both
        send_to_client(inviter->fd, "GAME_START#");
        send_to_client(client->fd, "GAME_START#");
        
        printf("Game started between %s and %s\n", 
               inviter->username, client->username);
    } else {
        send_to_client(client->fd, "ERROR:Inviter not found#");
    }
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE DECLINE_INVITE ====================
void handle_decline_invite(Client* client, char* buffer) {
    int inviter_id;
    sscanf(buffer+15, "%d", &inviter_id);

    pthread_mutex_lock(&clients_lock);
    Client* inviter = find_client_by_user_id(inviter_id);

    if(inviter) {
        // Send decline message to inviter
        char msg[256];
        snprintf(msg, sizeof(msg), "INVITE_DECLINED:%s#", client->username);
        send_to_client(inviter->fd, msg);

        // Clean up invite state
        client->invited_by = -1;
        inviter->opponent_id = -1;

        printf("%s (id=%d) declined invite from %s (id=%d)\n",
               client->username, client->user_id, inviter->username, inviter_id);
    } else {
        printf("ERROR: Inviter with id=%d not found\n", inviter_id);
    }
    pthread_mutex_unlock(&clients_lock);

    // Send confirmation to decliner
    send_to_client(client->fd, "DECLINE_OK#");
}

// ==================== HANDLE CANCEL_INVITE ====================
void handle_cancel_invite(Client* client) {
    pthread_mutex_lock(&clients_lock);
    // Find who was invited by this client
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i].fd > 0 && clients[i].invited_by == client->user_id) {
            send_to_client(clients[i].fd, "INVITE_CANCELLED#");
            clients[i].invited_by = -1;
        }
    }
    pthread_mutex_unlock(&clients_lock);
    send_to_client(client->fd, "INVITE_CANCEL_OK#");
}

// ==================== HANDLE PLACE ====================
void handle_place(Client* client, char* buffer) {
    if(!client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        return;
    }

    int length, x, y;
    char dir;
    if(sscanf(buffer+6, "%d,%d,%d,%c", &length, &x, &y, &dir) != 4) {
        send_to_client(client->fd, "ERROR:Invalid format#");
        return;
    }

    printf("DEBUG: %s PLACE len=%d x=%d y=%d dir=%c\n",
           client->username, length, x, y, dir);

    // Convert to 0-indexed
    x--;
    y--;

    // Normalize direction
    if(dir >= 'a' && dir <= 'z') dir = dir - 'a' + 'A';
    if(dir != 'H' && dir != 'V') {
        send_to_client(client->fd, "ERROR:Invalid direction#");
        return;
    }

    // Check ship count limits (1x4, 1x3, 2x2)
    int max_ships = (length == 4 || length == 3) ? 1 : 2;
    int count = 0;
    for(int i=0; i<client->ship_count; i++) {
        if(client->ships[i].length == length) count++;
    }
    if(count >= max_ships) {
        send_to_client(client->fd, "ERROR:Already placed all ships of this type#");
        return;
    }

    // Calculate end position
    int dx = (dir == 'H') ? 1 : 0;
    int dy = (dir == 'V') ? 1 : 0;
    int ex = x + dx * (length - 1);
    int ey = y + dy * (length - 1);

    // Check bounds
    if(x < 0 || y < 0 || ex < 0 || ey < 0 ||
       x >= MAP_SIZE || y >= MAP_SIZE ||
       ex >= MAP_SIZE || ey >= MAP_SIZE) {
        send_to_client(client->fd, "ERROR:Out of bounds#");
        return;
    }

    // Check overlap
    int overlap = 0;
    for(int i=0; i<length; i++) {
        int tx = x + dx * i;
        int ty = y + dy * i;
        if(client->map[ty][tx] != '-') {
            overlap = 1;
            break;
        }
    }
    if(overlap) {
        send_to_client(client->fd, "ERROR:Ship overlaps#");
        return;
    }

    // Place the ship
    pthread_mutex_lock(&clients_lock);

    Ship *sh = &client->ships[client->ship_count];
    sh->length = length;
    sh->hits = 0;
    sh->alive = 1;

    for(int i=0; i<length; i++) {
        int tx = x + dx * i;
        int ty = y + dy * i;
        client->map[ty][tx] = '0' + length;
        sh->x[i] = tx;
        sh->y[i] = ty;
    }

    client->ship_count++;
    pthread_mutex_unlock(&clients_lock);

    send_to_client(client->fd, "PLACE_OK:#");
    send_state(client);
}

// ==================== HANDLE READY ====================
void handle_ready(Client* client) {
    pthread_mutex_lock(&clients_lock);

    if(!client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }

    // Check if all ships are placed (1x4, 1x3, 2x2)
    if(!check_ready_ships(client)) {
        send_to_client(client->fd, "ERROR:Place all ships first#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }

    client->is_ready = 1;
    send_to_client(client->fd, "READY_OK:#");
    printf("%s is READY\n", client->username);

    // Check if opponent is also ready
    Client* opponent = find_client_by_user_id(client->opponent_id);
    if(opponent && opponent->is_ready) {
        // Both players ready - start playing
        printf("✓ Both players ready! Game starting: %s vs %s\n",
               client->username, opponent->username);

        usleep(100000); // Small delay
        send_to_client(client->fd, "START_PLAYING#");
        send_to_client(opponent->fd, "START_PLAYING#");
        
        // Set game start time
        time_t now = time(NULL);
        client->game_start_time = now;
        opponent->game_start_time = now;

        // Send initial state to both
        send_state(client);
        send_state(opponent);

        // Determine who goes first (inviter goes first)
        Client* first_player = (client->invited_by > 0) ? opponent : client;
        Client* second_player = (client->invited_by > 0) ? client : opponent;

        first_player->is_my_turn = 1;
        second_player->is_my_turn = 0;

        send_to_client(first_player->fd, "YOUR_TURN:#");
        send_to_client(second_player->fd, "WAIT_YOUR_TURN:#");

        printf("✓ %s goes first\n", first_player->username);
    } else {
        // Wait for opponent
        send_to_client(client->fd, "WAITING_OPPONENT#");
        if(opponent) {
            send_to_client(opponent->fd, "OPPONENT_READY#");
        }
    }

    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE FIRE ====================
void handle_fire(Client* client, char* buffer) {
    int x, y;
    if(sscanf(buffer+5, "%d,%d", &x, &y) != 2) {
        send_to_client(client->fd, "ERROR:Invalid format#");
        return;
    }

    // Convert to 0-indexed
    x--;
    y--;

    if(x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE) {
        send_to_client(client->fd, "ERROR:Out of bounds#");
        return;
    }

    pthread_mutex_lock(&clients_lock);

    // Check if it's their turn
    if(!client->is_my_turn) {
        send_to_client(client->fd, "WAIT_YOUR_TURN:#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }

    // Get opponent
    Client* opponent = find_client_by_user_id(client->opponent_id);
    if(!opponent) {
        send_to_client(client->fd, "ERROR:Opponent not found#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }

    // Check if already fired
    char already = client->enemy_map[y][x];
    if(already == 'x' || already == 'o' || already == '@') {
        send_to_client(client->fd, "ERROR:Already fired#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }

    // Process the fire
    char cell = opponent->map[y][x];
    
    // Track shot
    client->total_shots++;

    if(cell == '-') {
        // MISS
        client->enemy_map[y][x] = 'x';

        char res[64];
        snprintf(res, sizeof(res), "RESULT:MISS,%d,%d#", x+1, y+1);
        send_to_client(client->fd, res);
        printf("DEBUG: %s MISS at (%d,%d)\n", client->username, x+1, y+1);

        // Switch turn
        client->is_my_turn = 0;
        opponent->is_my_turn = 1;

        send_state(client);
        send_state(opponent);

        send_to_client(opponent->fd, "YOUR_TURN:#");
        send_to_client(client->fd, "WAIT_YOUR_TURN:#");
    }
    else if(cell >= '2' && cell <= '9') {
        // HIT
        client->enemy_map[y][x] = 'o';
        opponent->map[y][x] = 'o';
        
        // Track hit
        client->total_hits++;

        // Find and update ship
        for(int s=0; s<opponent->ship_count; s++) {
            Ship *sh = &opponent->ships[s];
            for(int k=0; k<sh->length; k++) {
                if(sh->x[k] == x && sh->y[k] == y) {
                    sh->hits++;
                    if(sh->hits == sh->length) {
                        sh->alive = 0;
                        mark_sunk(opponent, client);
                        printf("DEBUG: %s sunk a ship (length %d)\n",
                               client->username, sh->length);
                    }
                    break;
                }
            }
        }

        char res[64];
        snprintf(res, sizeof(res), "RESULT:HIT,%d,%d#", x+1, y+1);
        send_to_client(client->fd, res);
        printf("DEBUG: %s HIT at (%d,%d)\n", client->username, x+1, y+1);

        // Check if all opponent ships are sunk
        int opponent_alive = 0;
        for(int s=0; s<opponent->ship_count; s++) {
            if(opponent->ships[s].alive) {
                opponent_alive = 1;
                break;
            }
        }

        if(!opponent_alive) {
            // Game over - client wins
            printf("✓ Game over: %s wins!\n", client->username);
            
            // Calculate game duration
            time_t now = time(NULL);
            int game_duration = (int)difftime(now, client->game_start_time);
            
            // Calculate accuracy
            float client_accuracy = client->total_shots > 0 ? 
                (float)client->total_hits / client->total_shots * 100.0f : 0.0f;
            float opponent_accuracy = opponent->total_shots > 0 ? 
                (float)opponent->total_hits / opponent->total_shots * 100.0f : 0.0f;
            
            // Save match to database
            MatchHistory match = {0};
            match.player1_id = client->user_id;
            match.player2_id = opponent->user_id;
            match.winner_id = client->user_id;
            match.player1_total_shots = client->total_shots;
            match.player2_total_shots = opponent->total_shots;
            match.player1_hit_diff = client->total_hits - opponent->total_hits;
            match.player2_hit_diff = opponent->total_hits - client->total_hits;
            match.player1_accuracy = client_accuracy;
            match.player2_accuracy = opponent_accuracy;
            match.game_duration_seconds = game_duration;
            
            // Calculate and update ELO
            db_update_elo_after_match(&match);
            db_save_match(&match);
            
            // Update user stats
            db_update_score(client->user_id, 1, 1);  // Win
            db_update_score(opponent->user_id, 0, 0);  // Loss
            
            printf("  Game Stats: Duration=%ds, %s: %d shots %.1f%% acc, %s: %d shots %.1f%% acc\n",
                   game_duration, client->username, client->total_shots, client_accuracy,
                   opponent->username, opponent->total_shots, opponent_accuracy);
            
            // Send results with ELO change
            char win_msg[256];
            snprintf(win_msg, sizeof(win_msg), "YOU WIN:ELO %+d#", match.player1_elo_gain);
            send_to_client(client->fd, win_msg);
            
            char lose_msg[256];
            snprintf(lose_msg, sizeof(lose_msg), "YOU LOSE:ELO %+d#", match.player2_elo_gain);
            send_to_client(opponent->fd, lose_msg);

            // Reset game state
            client->in_game = 0;
            opponent->in_game = 0;
            client->is_ready = 0;
            opponent->is_ready = 0;
            client->opponent_id = -1;
            opponent->opponent_id = -1;
        }
        else {
            // Continue - attacker gets another turn
            send_to_client(client->fd, "HIT_CONTINUE:#");
            send_to_client(opponent->fd, "OPPONENT_HIT_CONTINUE:#");
        }

        send_state(client);
        send_state(opponent);
    }

    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE GET_LEADERBOARD ====================
void handle_get_leaderboard(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }

    char leaderboard_data[BUFF_SIZE];
    if(db_get_leaderboard(leaderboard_data, sizeof(leaderboard_data)) == 0) {
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response), "LEADERBOARD:%s#", leaderboard_data);
        send_to_client(client->fd, response);
    } else {
        send_to_client(client->fd, "ERROR:Failed to get leaderboard#");
    }
}

// ==================== HANDLE LOGOUT ====================
void handle_logout(Client* client) {
    if(client->user_id > 0) {
        db_logout_user(client->user_id);
        printf("User logged out: %s\n", client->username);
    }

    pthread_mutex_lock(&clients_lock);
    client->user_id = -1;
    client->is_authenticated = 0;
    pthread_mutex_unlock(&clients_lock);

    send_to_client(client->fd, "LOGOUT_OK#");
}

// ==================== CLIENT HANDLER ====================
void* client_handler(void* arg) {
    Client* client = (Client*)arg;
    char buffer[BUFF_SIZE];
    
    printf("New client connected (fd=%d)\n", client->fd);
    send_to_client(client->fd, "WELCOME#");
    
    while(1) {
        int n = recv(client->fd, buffer, BUFF_SIZE-1, 0);
        if(n <= 0) {
            printf("Client disconnected (fd=%d)\n", client->fd);
            break;
        }
        
        buffer[n] = '\0';
        printf("RECEIVED from fd=%d: %s\n", client->fd, buffer);
        
        // Parse commands
        if(strncmp(buffer, "REGISTER:", 9) == 0) {
            handle_register(client, buffer);
        }
        else if(strncmp(buffer, "LOGIN:", 6) == 0) {
            handle_login(client, buffer);
        }
        else if(strcmp(buffer, "GET_USERS#") == 0) {
            handle_get_users(client);
        }
        else if(strcmp(buffer, "GET_MY_STATS#") == 0) {
            handle_get_my_stats(client);
        }
        else if(strncmp(buffer, "INVITE:", 7) == 0) {
            handle_invite(client, buffer);
        }
        else if(strncmp(buffer, "ACCEPT_INVITE:", 14) == 0) {
            handle_accept_invite(client, buffer);
        }
        else if(strncmp(buffer, "DECLINE_INVITE:", 15) == 0) {
            handle_decline_invite(client, buffer);
        }
        else if(strcmp(buffer, "CANCEL_INVITE#") == 0) {
            handle_cancel_invite(client);
        }
        else if(strncmp(buffer, "PLACE:", 6) == 0) {
            handle_place(client, buffer);
        }
        else if(strcmp(buffer, "READY#") == 0) {
            handle_ready(client);
        }
        else if(strncmp(buffer, "FIRE:", 5) == 0) {
            handle_fire(client, buffer);
        }
        else if(strcmp(buffer, "GET_LEADERBOARD#") == 0) {
            handle_get_leaderboard(client);
        }
        else if(strcmp(buffer, "LOGOUT#") == 0) {
            handle_logout(client);
        }
        else {
            send_to_client(client->fd, "ERROR:Unknown command#");
        }
    }
    
    // Cleanup
    if(client->user_id > 0) {
        db_logout_user(client->user_id);
    }
    
    pthread_mutex_lock(&clients_lock);
    close(client->fd);
    client->fd = -1;
    client->is_authenticated = 0;
    client->in_game = 0;
    client_count--;
    pthread_mutex_unlock(&clients_lock);
    
    return NULL;
}

// ==================== MAIN ====================
int main() {
    // Initialize database
    if(db_init() != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    // Message already printed by db_init()
    
    // Initialize clients
    for(int i=0; i<MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].user_id = -1;
        clients[i].is_authenticated = 0;
        clients[i].in_game = 0;
        clients[i].invited_by = -1;
    }
    
    // Create socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;

    // Use 127.0.0.1 (localhost) by default
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons(PORT);

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        fprintf(stderr, "Failed to bind to 127.0.0.1:%d\n", PORT);
        return 1;
    }

    listen(listenfd, 10);
    printf("\n=================================\n");
    printf("Server listening on 127.0.0.1:%d\n", PORT);
    printf("=================================\n");
    printf("Waiting for connections...\n\n");
    
    // Accept connections
    while(1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        
        if(connfd < 0) {
            perror("accept");
            continue;
        }
        
        // Find free slot
        pthread_mutex_lock(&clients_lock);
        int slot = -1;
        for(int i=0; i<MAX_CLIENTS; i++) {
            if(clients[i].fd == -1) {
                slot = i;
                break;
            }
        }
        
        if(slot == -1) {
            send(connfd, "SERVER_FULL#", 12, 0);
            close(connfd);
            pthread_mutex_unlock(&clients_lock);
            continue;
        }
        
        clients[slot].fd = connfd;
        client_count++;
        
        pthread_create(&clients[slot].thread, NULL, client_handler, &clients[slot]);
        pthread_detach(clients[slot].thread);
        
        pthread_mutex_unlock(&clients_lock);
        
        printf("Client %d connected (total: %d)\n", slot, client_count);
    }
    
    db_close();
    close(listenfd);
    return 0;
}

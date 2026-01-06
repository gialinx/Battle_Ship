// server_lobby.c - Server with Lobby system and invite mechanism
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "database.h"
#include "matchmaking.h"

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
    int last_opponent_id; // Lưu opponent_id sau khi game kết thúc (để rematch)
    int is_ready;    // đã sẵn sàng chơi chưa
    int wants_rematch; // 1 if player clicked rematch button
    pthread_t thread;

    // AFK detection
    time_t last_activity_time;  // Last time client sent any message
    int afk_warned;             // 1 if AFK warning has been sent

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
    
    // Shot history tracking (for match replay)
    char shot_log[4096];                 // Log của từng shot: "x,y,hit,ship_len,sunk;"
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
                "MY_STATS:%s:%d:%d:%d:%d:%d#",
                profile.username, profile.total_games,
                profile.wins, profile.losses, profile.elo_rating, client->user_id);
        send_to_client(client->fd, response);
        printf("Sent stats to %s: ELO=%d, Games=%d, Wins=%d, Losses=%d\n",
               profile.username, profile.elo_rating, profile.total_games, profile.wins, profile.losses);
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
                // Format: user_id,username,status,elo_rating,in_game
                offset += snprintf(response + offset, BUFF_SIZE - offset,
                    "%d,%s,%s,%d,%d:",
                    profile.user_id,
                    profile.username,
                    profile.status,
                    profile.elo_rating,
                    clients[i].in_game  // Add in_game status
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
        send_to_client(client->fd, "INVITE_FAIL:Player is busy in game#");
    } else if(client->in_game) {
        printf("INVITE DEBUG: You (%s) are already in game\n", client->username);
        send_to_client(client->fd, "INVITE_FAIL:You are already in game#");
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
        
        // Log shot: x,y,hit(0=miss),ship_len(0),sunk(0)
        char shot_entry[50];
        snprintf(shot_entry, sizeof(shot_entry), "%d,%d,0,0,0;", x, y);
        strcat(client->shot_log, shot_entry);

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
        int hit_ship_len = 0;
        int ship_sunk = 0;
        for(int s=0; s<opponent->ship_count; s++) {
            Ship *sh = &opponent->ships[s];
            for(int k=0; k<sh->length; k++) {
                if(sh->x[k] == x && sh->y[k] == y) {
                    sh->hits++;
                    hit_ship_len = sh->length;
                    if(sh->hits == sh->length) {
                        sh->alive = 0;
                        ship_sunk = 1;
                        mark_sunk(opponent, client);
                        printf("DEBUG: %s sunk a ship (length %d)\n",
                               client->username, sh->length);
                    }
                    break;
                }
            }
        }
        
        // Log shot: x,y,hit(1),ship_len,sunk
        char shot_entry[50];
        snprintf(shot_entry, sizeof(shot_entry), "%d,%d,1,%d,%d;", 
                x, y, hit_ship_len, ship_sunk);
        strcat(client->shot_log, shot_entry);

        char res[64];
        snprintf(res, sizeof(res), "RESULT:HIT,%d,%d#", x+1, y+1);
        send_to_client(client->fd, res);
        printf("DEBUG: %s HIT at (%d,%d)\n", client->username, x+1, y+1);

        // Check if all opponent ships are sunk
        printf("DEBUG: Checking if game over... opponent has %d ships\n", opponent->ship_count);
        int opponent_alive = 0;
        for(int s=0; s<opponent->ship_count; s++) {
            printf("DEBUG:   Ship %d: length=%d, alive=%d\n", 
                   s, opponent->ships[s].length, opponent->ships[s].alive);
            if(opponent->ships[s].alive) {
                opponent_alive = 1;
                break;
            }
        }
        printf("DEBUG: opponent_alive = %d\n", opponent_alive);

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
            
            // Build match_data with interleaved shots
            // Format: p1_shot1;p2_shot1;p1_shot2;p2_shot2;...
            // But we have sequential logs, so store as: P1_SHOTS|P2_SHOTS
            snprintf(match.match_data, sizeof(match.match_data), "%s|%s", 
                    client->shot_log, opponent->shot_log);
            
            printf("SERVER: Match data saved (%zu bytes)\n", strlen(match.match_data));
            
            // Calculate and update ELO
            db_update_elo_after_match(&match);
            
            printf("SERVER: Saving match - P1:%d vs P2:%d, Winner:%d, Duration:%ds\n",
                   match.player1_id, match.player2_id, match.winner_id, match.game_duration_seconds);
            
            int match_id = db_save_match(&match);
            if(match_id > 0) {
                printf("SERVER: Match saved successfully with ID %d\n", match_id);
            } else {
                printf("SERVER: ERROR - Failed to save match to database!\n");
            }
            
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

            // Save opponent_id before reset (for rematch)
            client->last_opponent_id = client->opponent_id;
            opponent->last_opponent_id = opponent->opponent_id;

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

// ==================== HANDLE GET_MATCH_HISTORY ====================
void handle_get_match_history(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }

    printf("SERVER: GET_MATCH_HISTORY request from user_id=%d (%s)\n", 
           client->user_id, client->username);

    MatchHistory* matches;
    int count = 0;
    
    if(db_get_match_history(client->user_id, &matches, &count) != 0) {
        printf("SERVER: Failed to get match history from database\n");
        send_to_client(client->fd, "ERROR:Failed to get match history#");
        return;
    }

    printf("SERVER: Found %d matches for user %s\n", count, client->username);

    // Build response: MATCH_HISTORY:count:match1_data|match2_data|...#
    char response[BUFF_SIZE * 2];
    int offset = 0;
    offset += snprintf(response + offset, sizeof(response) - offset, "MATCH_HISTORY:%d", count);

    for(int i = 0; i < count; i++) {
        MatchHistory* m = &matches[i];
        
        // Determine if this user was player1 or player2
        int is_player1 = (m->player1_id == client->user_id);
        int opponent_id = is_player1 ? m->player2_id : m->player1_id;
        int my_shots = is_player1 ? m->player1_total_shots : m->player2_total_shots;
        int my_hits = is_player1 ? (m->player1_total_shots > 0 ? (int)(m->player1_accuracy * my_shots / 100.0f) : 0) 
                                 : (m->player2_total_shots > 0 ? (int)(m->player2_accuracy * my_shots / 100.0f) : 0);
        int my_misses = my_shots - my_hits;
        int my_elo_change = is_player1 ? m->player1_elo_gain : m->player2_elo_gain;
        int is_win = (m->winner_id == client->user_id) ? 1 : 0;
        
        // Get opponent username
        UserProfile opp_profile;
        char opponent_name[50] = "Unknown";
        if(db_get_user_profile(opponent_id, &opp_profile) == 0) {
            strcpy(opponent_name, opp_profile.username);
        }
        
        printf("SERVER: Match %d - opponent: %s, result: %s, elo: %+d\n",
               m->match_id, opponent_name, is_win ? "WIN" : "LOSS", my_elo_change);
        
        // Format: |match_id,timestamp,opponent_id,opponent_name,is_win,hits,misses,elo_change,duration
        offset += snprintf(response + offset, sizeof(response) - offset, 
                          "|%d,%ld,%d,%s,%d,%d,%d,%+d,%d",
                          m->match_id, m->played_at, opponent_id, opponent_name,
                          is_win, my_hits, my_misses, my_elo_change, m->game_duration_seconds);
    }

    offset += snprintf(response + offset, sizeof(response) - offset, "#");
    
    printf("SERVER: Sending response: %s\n", response);
    send_to_client(client->fd, response);
    
    free(matches);
}

// ==================== HANDLE GET_MATCH_DETAIL ====================
void handle_get_match_detail(Client* client, const char* cmd) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }

    // Parse: GET_MATCH_DETAIL:match_id#
    int match_id;
    if(sscanf(cmd, "GET_MATCH_DETAIL:%d#", &match_id) != 1) {
        send_to_client(client->fd, "ERROR:Invalid match detail request#");
        return;
    }

    MatchHistory match;
    if(db_get_match_for_rewatch(match_id, &match) != 0) {
        send_to_client(client->fd, "ERROR:Match not found#");
        return;
    }

    // Verify this user was part of the match
    if(match.player1_id != client->user_id && match.player2_id != client->user_id) {
        send_to_client(client->fd, "ERROR:Access denied#");
        return;
    }

    // Parse match_data (format: "x,y,hit,ship_len,sunk;x,y,hit,ship_len,sunk;...")
    // match_data should contain alternating shots: p1_shot1;p2_shot1;p1_shot2;p2_shot2;...
    
    // Build response: MATCH_DETAIL:match_id:my_shots_data|opponent_shots_data#
    // shots_data format: x,y,hit,ship_len,sunk;x,y,hit,ship_len,sunk;...
    
    char response[BUFF_SIZE * 2];
    snprintf(response, sizeof(response), "MATCH_DETAIL:%d:%s#", match_id, match.match_data);
    send_to_client(client->fd, response);
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
        
        // Remove from matchmaking queue if in queue
        if(mm_is_in_queue(client->user_id)) {
            mm_remove_player(client->user_id);
            printf("[MM] Player %s removed from queue (logout)\n", client->username);
        }
    }

    pthread_mutex_lock(&clients_lock);
    client->user_id = -1;
    client->is_authenticated = 0;
    pthread_mutex_unlock(&clients_lock);

    send_to_client(client->fd, "LOGOUT_OK#");
}

// ==================== HANDLE FIND_MATCH ====================
void handle_find_match(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    // Get user profile for ELO and total_games
    UserProfile profile;
    if(db_get_user_profile(client->user_id, &profile) != 0) {
        send_to_client(client->fd, "MM_ERROR:Failed to get profile#");
        return;
    }
    
    // Add to matchmaking queue
    int result = mm_add_player(client->user_id, profile.username, 
                                profile.elo_rating, profile.total_games);
    
    if(result == 0) {
        send_to_client(client->fd, "MM_JOINED#");
        printf("[MM] Player %s joined matchmaking queue (ELO:%d, Games:%d)\n", 
               profile.username, profile.elo_rating, profile.total_games);
    } else {
        send_to_client(client->fd, "MM_ERROR:Already in queue#");
    }
}

// ==================== HANDLE CANCEL_MATCH ====================
void handle_cancel_match(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    int result = mm_remove_player(client->user_id);
    
    if(result == 0) {
        send_to_client(client->fd, "MM_CANCELLED#");
        printf("[MM] Player %s cancelled matchmaking\n", client->username);
    } else {
        send_to_client(client->fd, "MM_ERROR:Not in queue#");
    }
}

// ==================== HANDLE ACCEPT_MATCH ====================
void handle_accept_match(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    
    if(client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:No match found#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    Client* opponent = find_client_by_user_id(client->opponent_id);
    if(!opponent) {
        send_to_client(client->fd, "ERROR:Opponent disconnected#");
        client->opponent_id = -1;
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    // Mark this client as ready to start
    client->is_ready = 1;
    
    // Check if opponent also accepted
    if(opponent->is_ready == 1) {
        // Both accepted! Start the game
        client->in_game = 1;
        opponent->in_game = 1;
        
        // Reset game data for new match
        init_map(client->map);
        init_map(client->enemy_map);
        init_map(opponent->map);
        init_map(opponent->enemy_map);
        client->ship_count = 0;
        opponent->ship_count = 0;
        client->total_shots = 0;
        opponent->total_shots = 0;
        client->total_hits = 0;
        opponent->total_hits = 0;
        client->is_ready = 0;  // Reset for ship placement phase
        opponent->is_ready = 0;
        
        // Send confirmation to both
        send_to_client(client->fd, "MATCH_ACCEPTED:GAME_START#");
        send_to_client(opponent->fd, "MATCH_ACCEPTED:GAME_START#");
        
        printf("[MATCH] Both players accepted: %s vs %s - Game starting!\n", 
               client->username, opponent->username);
    } else {
        // Wait for opponent to accept
        send_to_client(client->fd, "MATCH_ACCEPTED:WAITING_OPPONENT#");
        send_to_client(opponent->fd, "OPPONENT_ACCEPTED#");
        printf("[MATCH] %s accepted, waiting for %s\n", client->username, opponent->username);
    }
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE DECLINE_MATCH ====================
void handle_decline_match(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    
    if(client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:No match found#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    Client* opponent = find_client_by_user_id(client->opponent_id);
    
    // Notify opponent that match was declined
    if(opponent) {
        char msg[256];
        snprintf(msg, sizeof(msg), "MATCH_DECLINED:%s#", client->username);
        send_to_client(opponent->fd, msg);
        opponent->opponent_id = -1;
        opponent->is_ready = 0;
        printf("[MATCH] %s declined match with %s\n", client->username, opponent->username);
    }
    
    // Clear match state
    send_to_client(client->fd, "MATCH_DECLINED:OK#");
    client->opponent_id = -1;
    client->is_ready = 0;
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== RESET GAME STATE ====================
void reset_game_state(Client* client) {
    client->in_game = 0;
    client->opponent_id = -1;
    client->is_ready = 0;
    client->is_my_turn = 0;
    client->wants_rematch = 0;
    client->ship_count = 0;
    client->total_shots = 0;
    client->total_hits = 0;
    client->game_start_time = 0;
    memset(client->shot_log, 0, sizeof(client->shot_log));
    init_map(client->map);
    init_map(client->enemy_map);
}

// ==================== HANDLE FORFEIT ====================
void handle_forfeit(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    if(!client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    Client* opponent = find_client_by_user_id(client->opponent_id);
    
    if(!opponent) {
        send_to_client(client->fd, "ERROR:Opponent not found#");
        reset_game_state(client);
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    // Determine game phase
    int during_placement = (!client->is_ready || !opponent->is_ready);
    
    if(during_placement) {
        // During placement - no ELO change, just cancel the match
        char msg[256];
        snprintf(msg, sizeof(msg), "OPPONENT_LEFT_PLACEMENT:%s#", client->username);
        send_to_client(opponent->fd, msg);
        send_to_client(client->fd, "FORFEIT_PLACEMENT#");
        
        // Reset both players
        reset_game_state(client);
        reset_game_state(opponent);
        
        printf("[FORFEIT] %s quit during placement vs %s (no ELO change)\n", 
               client->username, opponent->username);
    } else {
        // During active game - record as loss
        UserProfile winner_profile, loser_profile;
        db_get_user_profile(opponent->user_id, &winner_profile);
        db_get_user_profile(client->user_id, &loser_profile);
        
        // Calculate game duration
        time_t now = time(NULL);
        int duration = (client->game_start_time > 0) ? (now - client->game_start_time) : 0;
        
        // Create match history
        MatchHistory match;
        memset(&match, 0, sizeof(MatchHistory));
        
        match.player1_id = opponent->user_id;  // Winner
        match.player2_id = client->user_id;    // Loser
        match.winner_id = opponent->user_id;
        match.player1_score = 100;  // Winner gets full score
        match.player2_score = 0;    // Forfeit = 0 score
        
        // Calculate accuracy
        match.player1_total_shots = opponent->total_shots;
        match.player1_accuracy = (opponent->total_shots > 0) ? 
            ((float)opponent->total_hits / opponent->total_shots) : 0.0f;
        match.player1_hit_diff = opponent->total_hits;
        
        match.player2_total_shots = client->total_shots;
        match.player2_accuracy = (client->total_shots > 0) ?
            ((float)client->total_hits / client->total_shots) : 0.0f;
        match.player2_hit_diff = client->total_hits;
        
        match.game_duration_seconds = duration;
        
        // Copy shot logs
        snprintf(match.match_data, sizeof(match.match_data),
                "P1:%s;P2:%s;FORFEIT", opponent->shot_log, client->shot_log);
        
        match.played_at = now;
        
        // Save match (this will calculate and update ELO automatically)
        int match_id = db_save_match(&match);
        
        // Update wins/losses
        db_update_score(opponent->user_id, 100, 1);  // Win
        db_update_score(client->user_id, 0, 0);      // Loss
        
        // Get ELO changes from match struct (updated by db_save_match)
        int winner_change = match.player1_elo_gain;
        int loser_change = match.player2_elo_gain;
        
        // Send results
        char win_msg[512];
        snprintf(win_msg, sizeof(win_msg),
                "GAME_OVER:WIN:Opponent surrendered:%d:%d#",
                match.player1_elo_after, winner_change);
        send_to_client(opponent->fd, win_msg);
        
        char lose_msg[512];
        snprintf(lose_msg, sizeof(lose_msg),
                "GAME_OVER:LOSE:You surrendered:%d:%d#",
                match.player2_elo_after, loser_change);
        send_to_client(client->fd, lose_msg);
        
        printf("[FORFEIT] %s surrendered to %s (Match ID=%d, ELO change: %+d vs %+d)\n",
               client->username, opponent->username, match_id, loser_change, winner_change);
        
        // Save opponent_id before reset (for rematch)
        client->last_opponent_id = client->opponent_id;
        opponent->last_opponent_id = opponent->opponent_id;
        
        // Reset both players
        reset_game_state(client);
        reset_game_state(opponent);
    }
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE SURRENDER_REQUEST ====================
void handle_surrender_request(Client* client) {
    if(!client->is_authenticated || !client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    Client* opponent = find_client_by_user_id(client->opponent_id);
    
    if(!opponent) {
        send_to_client(client->fd, "ERROR:Opponent not found#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    // Send surrender request to opponent
    char msg[256];
    snprintf(msg, sizeof(msg), "SURRENDER_REQUEST_FROM:%s#", client->username);
    send_to_client(opponent->fd, msg);
    
    printf("[SURRENDER] %s requested surrender to %s\n", client->username, opponent->username);
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE SURRENDER_ACCEPT ====================
void handle_surrender_accept(Client* client) {
    if(!client->is_authenticated || !client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    Client* surrenderer = find_client_by_user_id(client->opponent_id);
    
    if(!surrenderer) {
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    // Client accepts, surrenderer loses
    UserProfile winner_profile, loser_profile;
    db_get_user_profile(client->user_id, &winner_profile);
    db_get_user_profile(surrenderer->user_id, &loser_profile);
    
    // Calculate game duration
    time_t now = time(NULL);
    int duration = (client->game_start_time > 0) ? (now - client->game_start_time) : 0;
    
    // Create match history
    MatchHistory match;
    memset(&match, 0, sizeof(MatchHistory));
    
    match.player1_id = client->user_id;      // Winner
    match.player2_id = surrenderer->user_id; // Loser (surrendered)
    match.winner_id = client->user_id;
    match.player1_score = 100;
    match.player2_score = 0;
    
    match.player1_total_shots = client->total_shots;
    match.player1_accuracy = (client->total_shots > 0) ? 
        ((float)client->total_hits / client->total_shots) : 0.0f;
    match.player1_hit_diff = client->total_hits;
    
    match.player2_total_shots = surrenderer->total_shots;
    match.player2_accuracy = (surrenderer->total_shots > 0) ?
        ((float)surrenderer->total_hits / surrenderer->total_shots) : 0.0f;
    match.player2_hit_diff = surrenderer->total_hits;
    
    match.game_duration_seconds = duration;
    
    snprintf(match.match_data, sizeof(match.match_data),
            "P1:%s;P2:%s;SURRENDER", client->shot_log, surrenderer->shot_log);
    
    match.played_at = now;
    
    // Save match (will calculate ELO)
    int match_id = db_save_match(&match);
    
    // Update wins/losses
    db_update_score(client->user_id, 100, 1);       // Win
    db_update_score(surrenderer->user_id, 0, 0);    // Loss
    
    // For surrender: winner still gets ELO based on accuracy, loser loses ELO as penalty
    int winner_change = match.player1_elo_gain;  // Winner ELO based on accuracy
    int loser_change = match.player2_elo_gain;   // Loser loses ELO
    
    // Send results
    char win_msg[512];
    snprintf(win_msg, sizeof(win_msg),
            "GAME_OVER:WIN:Opponent surrendered:%d:%d#",
            match.player1_elo_after, winner_change);
    send_to_client(client->fd, win_msg);
    
    char lose_msg[512];
    snprintf(lose_msg, sizeof(lose_msg),
            "GAME_OVER:LOSE:You surrendered:%d:%d#",
            match.player2_elo_after, loser_change);
    send_to_client(surrenderer->fd, lose_msg);
    
    printf("[SURRENDER] %s surrendered to %s (Match ID=%d)\n",
           surrenderer->username, client->username, match_id);
    
    // Save opponent_id before reset (for rematch)
    client->last_opponent_id = client->opponent_id;
    surrenderer->last_opponent_id = surrenderer->opponent_id;
    
    reset_game_state(client);
    reset_game_state(surrenderer);
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE SURRENDER_DECLINE ====================
void handle_surrender_decline(Client* client) {
    if(!client->is_authenticated || !client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    Client* surrenderer = find_client_by_user_id(client->opponent_id);
    
    if(surrenderer) {
        send_to_client(surrenderer->fd, "SURRENDER_DECLINED#");
        printf("[SURRENDER] %s declined surrender from %s\n", 
               client->username, surrenderer->username);
    }
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE REMATCH_REQUEST ====================
void handle_rematch_request(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    // Mark that this client wants rematch
    client->wants_rematch = 1;
    
    pthread_mutex_lock(&clients_lock);
    // Use last_opponent_id since opponent_id is reset after game over
    int opp_id = (client->opponent_id > 0) ? client->opponent_id : client->last_opponent_id;
    Client* opponent = find_client_by_user_id(opp_id);
    
    if(!opponent) {
        pthread_mutex_unlock(&clients_lock);
        send_to_client(client->fd, "ERROR:Opponent not found#");
        client->wants_rematch = 0;
        return;
    }
    
    // Check if opponent also wants rematch
    if(opponent->wants_rematch) {
        // Both want rematch! Start new game immediately
        printf("[REMATCH] Both %s and %s want rematch, starting new game\n",
               client->username, opponent->username);
        
        // Reset game state but keep opponent_id
        int saved_opp_client = client->opponent_id;
        int saved_opp_opponent = opponent->opponent_id;
        
        reset_game_state(client);
        reset_game_state(opponent);
        
        // Restore opponent relationship
        client->opponent_id = saved_opp_client;
        opponent->opponent_id = saved_opp_opponent;
        client->in_game = 1;
        opponent->in_game = 1;
        
        // Send GAME_START to both
        send_to_client(client->fd, "BOTH_WANT_REMATCH#");
        send_to_client(opponent->fd, "BOTH_WANT_REMATCH#");
        send_to_client(client->fd, "GAME_START#");
        send_to_client(opponent->fd, "GAME_START#");
    } else {
        // Opponent hasn't requested rematch yet, send request
        char msg[128];
        snprintf(msg, sizeof(msg), "REMATCH_REQUEST_FROM:%s#", client->username);
        send_to_client(opponent->fd, msg);
        send_to_client(client->fd, "WAITING_REMATCH_RESPONSE#");
        printf("[REMATCH] %s sent rematch request to %s\n", 
               client->username, opponent->username);
    }
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE REMATCH_ACCEPT ====================
void handle_rematch_accept(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    // Use last_opponent_id since opponent_id is reset after game over
    int opp_id = (client->opponent_id > 0) ? client->opponent_id : client->last_opponent_id;
    Client* opponent = find_client_by_user_id(opp_id);
    
    if(!opponent) {
        pthread_mutex_unlock(&clients_lock);
        send_to_client(client->fd, "ERROR:Opponent not found#");
        return;
    }
    
    printf("[REMATCH] %s accepted rematch from %s\n", 
           client->username, opponent->username);
    
    // Reset game state but keep opponent_id
    int saved_opp_client = opp_id;
    int saved_opp_opponent = (opponent->opponent_id > 0) ? opponent->opponent_id : opponent->last_opponent_id;
    
    reset_game_state(client);
    reset_game_state(opponent);
    
    // Restore opponent relationship
    client->opponent_id = saved_opp_client;
    opponent->opponent_id = saved_opp_opponent;
    client->in_game = 1;
    opponent->in_game = 1;
    
    // Send GAME_START to both
    send_to_client(client->fd, "GAME_START#");
    send_to_client(opponent->fd, "GAME_START#");
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE REMATCH_DECLINE ====================
void handle_rematch_decline(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    pthread_mutex_lock(&clients_lock);
    int opp_id = (client->opponent_id > 0) ? client->opponent_id : client->last_opponent_id;
    Client* opponent = find_client_by_user_id(opp_id);
    
    if(opponent) {
        send_to_client(opponent->fd, "REMATCH_DECLINED#");
        printf("[REMATCH] %s declined rematch from %s\n", 
               client->username, opponent->username);
        opponent->wants_rematch = 0;
    }
    
    // Reset both to lobby
    reset_game_state(client);
    if(opponent) {
        reset_game_state(opponent);
    }
    
    pthread_mutex_unlock(&clients_lock);
}

// ==================== HANDLE CANCEL_REMATCH ====================
void handle_cancel_rematch(Client* client) {
    if(!client->is_authenticated) {
        send_to_client(client->fd, "ERROR:Not authenticated#");
        return;
    }
    
    client->wants_rematch = 0;
    printf("[REMATCH] %s cancelled rematch request\n", client->username);
    
    // Client goes back to lobby
    reset_game_state(client);
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
        
        // Update last activity time for AFK detection
        client->last_activity_time = time(NULL);
        client->afk_warned = 0;  // Reset warning flag on any activity
        
        // Split buffer by '#' to handle multiple commands in one packet
        char buffer_copy[BUFF_SIZE];
        strncpy(buffer_copy, buffer, BUFF_SIZE - 1);
        buffer_copy[BUFF_SIZE - 1] = '\0';
        
        char* saveptr = NULL;
        char* token = strtok_r(buffer_copy, "#", &saveptr);
        
        while(token != NULL) {
            // Add '#' back for strcmp compatibility
            char cmd[BUFF_SIZE];
            snprintf(cmd, sizeof(cmd), "%s#", token);
            
            // Parse commands
            if(strncmp(cmd, "REGISTER:", 9) == 0) {
                handle_register(client, cmd);
            }
            else if(strncmp(cmd, "LOGIN:", 6) == 0) {
                handle_login(client, cmd);
            }
            else if(strcmp(cmd, "GET_USERS#") == 0) {
                handle_get_users(client);
            }
            else if(strcmp(cmd, "GET_MY_STATS#") == 0) {
                handle_get_my_stats(client);
            }
            else if(strncmp(cmd, "INVITE:", 7) == 0) {
                handle_invite(client, cmd);
            }
            else if(strncmp(cmd, "ACCEPT_INVITE:", 14) == 0) {
                handle_accept_invite(client, cmd);
            }
            else if(strncmp(cmd, "DECLINE_INVITE:", 15) == 0) {
                handle_decline_invite(client, cmd);
            }
            else if(strcmp(cmd, "CANCEL_INVITE#") == 0) {
                handle_cancel_invite(client);
            }
            else if(strncmp(cmd, "PLACE:", 6) == 0) {
                handle_place(client, cmd);
            }
            else if(strcmp(cmd, "READY#") == 0) {
                handle_ready(client);
            }
            else if(strncmp(cmd, "FIRE:", 5) == 0) {
                handle_fire(client, cmd);
            }
            else if(strcmp(cmd, "GET_LEADERBOARD#") == 0) {
                handle_get_leaderboard(client);
            }
            else if(strcmp(cmd, "GET_MATCH_HISTORY#") == 0) {
                handle_get_match_history(client);
            }
            else if(strncmp(cmd, "GET_MATCH_DETAIL:", 17) == 0) {
                handle_get_match_detail(client, cmd);
            }
            else if(strcmp(cmd, "FIND_MATCH#") == 0) {
                handle_find_match(client);
            }
            else if(strcmp(cmd, "CANCEL_MATCH#") == 0) {
                handle_cancel_match(client);
            }
            else if(strcmp(cmd, "ACCEPT_MATCH#") == 0) {
                handle_accept_match(client);
            }
            else if(strcmp(cmd, "DECLINE_MATCH#") == 0) {
                handle_decline_match(client);
            }
            else if(strcmp(cmd, "FORFEIT#") == 0) {
                handle_forfeit(client);
            }
            else if(strcmp(cmd, "SURRENDER_REQUEST#") == 0) {
                handle_surrender_request(client);
            }
            else if(strcmp(cmd, "SURRENDER_ACCEPT#") == 0) {
                handle_surrender_accept(client);
            }
            else if(strcmp(cmd, "SURRENDER_DECLINE#") == 0) {
                handle_surrender_decline(client);
            }
            else if(strcmp(cmd, "REMATCH_REQUEST#") == 0) {
                handle_rematch_request(client);
            }
            else if(strcmp(cmd, "REMATCH_ACCEPT#") == 0) {
                handle_rematch_accept(client);
            }
            else if(strcmp(cmd, "REMATCH_DECLINE#") == 0) {
                handle_rematch_decline(client);
            }
            else if(strcmp(cmd, "CANCEL_REMATCH#") == 0) {
                handle_cancel_rematch(client);
            }
            else if(strcmp(cmd, "LOGOUT#") == 0) {
                handle_logout(client);
            }
            else if(strcmp(cmd, "PONG#") == 0) {
                // Heartbeat response - already updated last_activity_time above
                // No response needed
            }
            else if(strcmp(cmd, "AFK_RESPONSE#") == 0) {
                // Player responded to AFK warning
                client->afk_warned = 0;
                printf("[AFK] %s responded to AFK warning\n", client->username);
            }
            else if(strlen(token) > 0) {  // Only report error if token is not empty
                send_to_client(client->fd, "ERROR:Unknown command#");
            }
            
            // Get next token
            token = strtok_r(NULL, "#", &saveptr);
        }
    }
    
    // Cleanup
    if(client->user_id > 0) {
        mm_remove_player(client->user_id);  // Remove from matchmaking queue if present
        db_logout_user(client->user_id);
    }
    
    pthread_mutex_lock(&clients_lock);
    close(client->fd);
    client->fd = -1;
    client->is_authenticated = 0;
    client->in_game = 0;
    pthread_mutex_unlock(&clients_lock);
    
    return NULL;
}

// ==================== MATCHMAKING THREAD ====================
void* matchmaking_thread(void *arg) {
    (void)arg;  // Suppress unused warning
    
    while(1) {
        sleep(2);  // Poll every 2 seconds
        
        // Cleanup timed-out players
        mm_cleanup_timeout();
        
        // Try to find matches
        int player1_id, player2_id;
        while(mm_find_any_match(&player1_id, &player2_id) == 0) {
            // Found a match! Find both clients
            Client *client1 = NULL;
            Client *client2 = NULL;
            
            pthread_mutex_lock(&clients_lock);
            for(int i = 0; i < MAX_CLIENTS; i++) {
                if(clients[i].fd != -1 && clients[i].user_id == player1_id) {
                    client1 = &clients[i];
                }
                if(clients[i].fd != -1 && clients[i].user_id == player2_id) {
                    client2 = &clients[i];
                }
            }
            
            if(client1 && client2) {
                // Get player info from database
                UserProfile profile1, profile2;
                if(db_get_user_profile(player1_id, &profile1) == 0 &&
                   db_get_user_profile(player2_id, &profile2) == 0) {
                    
                    // Send MATCH_FOUND to both clients (with opponent_id)
                    char msg1[256], msg2[256];
                    snprintf(msg1, sizeof(msg1), "MATCH_FOUND:%s:%d:%d#", 
                             profile2.username, player2_id, profile2.elo_rating);
                    snprintf(msg2, sizeof(msg2), "MATCH_FOUND:%s:%d:%d#", 
                             profile1.username, player1_id, profile1.elo_rating);
                    
                    send(client1->fd, msg1, strlen(msg1), 0);
                    send(client2->fd, msg2, strlen(msg2), 0);
                    
                    // Store opponent info but don't start game yet (wait for accept)
                    client1->opponent_id = player2_id;
                    client2->opponent_id = player1_id;
                    client1->in_game = 0;  // Will be set to 1 when both accept
                    client2->in_game = 0;
                    client1->is_ready = 0;
                    client2->is_ready = 0;
                    
                    // Reset game data for new match
                    init_map(client1->map);
                    init_map(client1->enemy_map);
                    init_map(client2->map);
                    init_map(client2->enemy_map);
                    client1->ship_count = 0;
                    client2->ship_count = 0;
                    client1->total_shots = 0;
                    client2->total_shots = 0;
                    client1->total_hits = 0;
                    client2->total_hits = 0;
                    
                    printf("[MATCHMAKING] Match created: %s (ELO %d) vs %s (ELO %d)\n",
                           profile1.username, profile1.elo_rating,
                           profile2.username, profile2.elo_rating);
                }
            }
            pthread_mutex_unlock(&clients_lock);
        }
    }
    return NULL;
}

// ==================== AFK DETECTION THREAD ====================
void* afk_detection_thread(void* arg) {
    (void)arg;
    
    const int AFK_WARNING_TIMEOUT = 180;  // 3 minutes (180 seconds)
    const int AFK_FORFEIT_TIMEOUT = 300;  // 5 minutes total (300 seconds)
    
    while(1) {
        sleep(30);  // Check every 30 seconds
        
        time_t now = time(NULL);
        
        pthread_mutex_lock(&clients_lock);
        for(int i = 0; i < MAX_CLIENTS; i++) {
            Client* client = &clients[i];
            
            // Only check clients in active game
            if(client->fd == -1 || !client->is_authenticated || !client->in_game) {
                continue;
            }
            
            if(client->last_activity_time == 0) {
                client->last_activity_time = now;
                continue;
            }
            
            int idle_time = (int)(now - client->last_activity_time);
            
            // Check for forfeit timeout (5 minutes total)
            if(idle_time >= AFK_FORFEIT_TIMEOUT && client->afk_warned) {
                printf("[AFK] %s has been AFK for %d seconds - auto forfeiting\n", 
                       client->username, idle_time);
                
                // Unlock before calling handle_forfeit to avoid deadlock
                pthread_mutex_unlock(&clients_lock);
                handle_forfeit(client);
                pthread_mutex_lock(&clients_lock);
                continue;
            }
            
            // Check for warning timeout (3 minutes)
            if(idle_time >= AFK_WARNING_TIMEOUT && !client->afk_warned) {
                printf("[AFK] %s has been idle for %d seconds - sending warning\n", 
                       client->username, idle_time);
                
                send_to_client(client->fd, "AFK_WARNING#");
                client->afk_warned = 1;
            }
        }
        pthread_mutex_unlock(&clients_lock);
    }
    
    return NULL;
}

// ==================== MAIN ====================
int main() {
    printf("========================================\n");
    printf("BATTLESHIP SERVER WITH MATCH HISTORY\n");
    printf("Version: 2026-01-05 with AFK detection\n");
    printf("========================================\n");
    
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
    
    // Start matchmaking thread
    pthread_t mm_thread;
    pthread_create(&mm_thread, NULL, matchmaking_thread, NULL);
    pthread_detach(mm_thread);
    printf("[MATCHMAKING] Background thread started\n\n");
    
    // Start AFK detection thread
    pthread_t afk_thread;
    pthread_create(&afk_thread, NULL, afk_detection_thread, NULL);
    pthread_detach(afk_thread);
    printf("[AFK DETECTION] Background thread started\n\n");
    
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

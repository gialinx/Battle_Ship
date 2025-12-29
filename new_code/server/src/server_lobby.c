// server_lobby.c - Server with Lobby system and invite mechanism
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "database.h"

#define PORT 5500
#define MAX_CLIENTS 50
#define BUFF_SIZE 8192

// Client structure
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
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

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
    Client* target = find_client_by_user_id(target_user_id);

    if(target && target->is_authenticated && !target->in_game) {
        // Send invite to target
        char msg[256];
        snprintf(msg, sizeof(msg), "INVITE_FROM:%d:%s#",
                 client->user_id, client->username);
        send_to_client(target->fd, msg);

        // Store who invited whom
        target->invited_by = client->user_id;
        client->opponent_id = target_user_id;  // Store who was invited by this client

        send_to_client(client->fd, "INVITE_SENT#");
        printf("Invite sent from %s (id=%d) to %s (id=%d)\n",
               client->username, client->user_id, target->username, target_user_id);
    } else {
        send_to_client(client->fd, "INVITE_FAIL:User not available#");
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
        client->invited_by = -1;
        client->opponent_id = inviter_id;
        inviter->opponent_id = client->user_id;
        client->is_ready = 0;
        inviter->is_ready = 0;
        
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

// ==================== HANDLE READY ====================
void handle_ready(Client* client) {
    pthread_mutex_lock(&clients_lock);
    
    if(!client->in_game || client->opponent_id <= 0) {
        send_to_client(client->fd, "ERROR:Not in game#");
        pthread_mutex_unlock(&clients_lock);
        return;
    }
    
    client->is_ready = 1;
    printf("%s is READY\n", client->username);
    
    // Check if opponent is also ready
    Client* opponent = find_client_by_user_id(client->opponent_id);
    if(opponent && opponent->is_ready) {
        // Both players ready - start playing
        send_to_client(client->fd, "START_PLAYING#");
        send_to_client(opponent->fd, "START_PLAYING#");
        printf("Both players ready! Game starting: %s vs %s\n", 
               client->username, opponent->username);
    } else {
        // Wait for opponent
        send_to_client(client->fd, "WAITING_OPPONENT#");
        if(opponent) {
            send_to_client(opponent->fd, "OPPONENT_READY#");
        }
    }
    
    pthread_mutex_unlock(&clients_lock);
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
        else if(strcmp(buffer, "READY#") == 0) {
            handle_ready(client);
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
    printf("Database initialized successfully\n");
    
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
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return 1;
    }
    
    listen(listenfd, 10);
    printf("Server listening at 127.0.0.1:%d\n", PORT);
    printf("Waiting for connections...\n");
    
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

// client_gui_complete.c - Complete client using modular architecture
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../core/game_data.h"
#include "../ui/renderer.h"
#include "../ui/colors.h"
#include "../ui/screens/login_screen.h"
#include "../ui/screens/lobby_screen.h"
#include "../ui/screens/profile_screen.h"
#include "../ui/screens/matchmaking_screen.h"
#include "../ui/screens/match_found_screen.h"
#include "../ui/screens/invitation_screen.h"
#include "../ui/screens/placing_ships_screen.h"
#include "../ui/screens/playing_screen.h"
#include "../ui/screens/game_over_screen.h"
#include "../network/network.h"
#include "../network/protocol.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

GameData game;
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;

// Global variable for server IP
const char* server_ip = "127.0.0.1"; // Default to localhost

// ==================== INITIALIZE GAME ====================
void init_game() {
    fprintf(stderr, "DEBUG: Starting SDL_Init...\n");
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        exit(1);
    }
    fprintf(stderr, "DEBUG: SDL_Init OK\n");

    fprintf(stderr, "DEBUG: Starting TTF_Init...\n");
    if(TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        exit(1);
    }
    fprintf(stderr, "DEBUG: TTF_Init OK\n");

    fprintf(stderr, "DEBUG: Creating window...\n");
    game.window = SDL_CreateWindow("Battleship - Complete",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if(!game.window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        exit(1);
    }
    fprintf(stderr, "DEBUG: Window created OK\n");

    fprintf(stderr, "DEBUG: Creating renderer...\n");
    game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED);
    if(!game.renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        exit(1);
    }
    fprintf(stderr, "DEBUG: Renderer created OK\n");

    fprintf(stderr, "DEBUG: Loading fonts...\n");
    game.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);
    if(!game.font) {
        fprintf(stderr, "TTF_OpenFont Error (Bold): %s\n", TTF_GetError());
        exit(1);
    }

    game.font_small = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    if(!game.font_small) {
        fprintf(stderr, "TTF_OpenFont Error (Small): %s\n", TTF_GetError());
        exit(1);
    }
    fprintf(stderr, "DEBUG: Fonts loaded OK\n");

    // Khởi tạo hệ thống assets (ảnh và âm thanh)
    if(!assets_init()) {
        fprintf(stderr, "ERROR: Failed to initialize assets system!\n");
        exit(1);
    }
    assets_manager_init(&game.assets);
    
    // Load ảnh nền cho màn hình login
    assets_load_image(&game.assets, game.renderer, "battle_ship_1.png");

    // TODO: Load thêm ảnh và âm thanh khác ở đây
    // VD: assets_load_image(&game.assets, game.renderer, "explosion.png");
    // VD: assets_load_sound(&game.assets, "shot.wav");

    game.state = STATE_LOGIN;
    game.running = 1;
    game.is_register_mode = 0;
    
    // Setup input fields - IMPORTANT: Initialize text to empty
    game.username_field.text[0] = '\0';
    game.username_field.cursor_pos = 0;
    game.username_field.rect = (SDL_Rect){300, 250, 400, 40};
    game.username_field.is_password = 0;
    game.username_field.is_active = 1;
    
    game.password_field.text[0] = '\0';
    game.password_field.cursor_pos = 0;
    game.password_field.rect = (SDL_Rect){300, 320, 400, 40};
    game.password_field.is_password = 1;
    game.password_field.is_active = 0;

    // Setup player search field (in lobby)
    int list_x = SCREEN_WIDTH - 300; // PLAYER_LIST_WIDTH = 300
    game.player_search_field.rect = (SDL_Rect){list_x + 10, 120, 280, 30};
    game.player_search_field.is_password = 0;
    game.player_search_field.is_active = 0;
    game.player_search_field.text[0] = '\0';

    strcpy(game.login_message, "");
    
    // Initialize maps
    for(int i=0; i<MAP_SIZE; i++)
        for(int j=0; j<MAP_SIZE; j++) {
            game.own_map[i][j] = '-';
            game.enemy_map[i][j] = '-';
        }
    
    // Initialize ship placement
    placing_ships_init(&game);

    // Initialize lobby tab
    game.active_lobby_tab = LOBBY_TAB_LEADERBOARD;
    game.losses = 0;
    game.rank = 0;
    game.leaderboard_count = 0;
    game.last_leaderboard_update = 0;
    game.last_users_update = 0;
    game.match_history_count = 0;
    game.matchmaking_active = 0;
    game.logout_requested = 0;

    // Connect to server using configured IP
    printf("Connecting to server at %s:%d...\n", server_ip, PORT);
    game.sockfd = connect_to_server(server_ip, PORT);
    if(game.sockfd < 0) {
        printf("Connection failed to %s:%d!\n", server_ip, PORT);
        printf("Make sure the server is running and accessible.\n");
        exit(1);
    }

    printf("Connected to server at %s:%d\n", server_ip, PORT);
}

// ==================== SEND TO SERVER ====================
void send_msg(const char* msg) {
    char buffer[BUFF_SIZE];
    snprintf(buffer, sizeof(buffer), "%s#", msg);
    send(game.sockfd, buffer, strlen(buffer), 0);
    printf("SENT: %s\n", buffer);
}

// ==================== RECEIVE THREAD ====================
void* receive_thread(void* arg) {
    (void)arg;
    char buffer[BUFF_SIZE];
    
    while(game.running) {
        int n = recv(game.sockfd, buffer, BUFF_SIZE-1, 0);
        if(n <= 0) break;
        
        buffer[n] = '\0';
        printf("RECEIVED: %s\n", buffer);
        
        pthread_mutex_lock(&game_lock);
        
        // Parse messages separated by '#'
        char* saveptr = NULL;
        char* token = strtok_r(buffer, "#", &saveptr);
        while(token) {
            parse_server_message(&game, token);
            token = strtok_r(NULL, "#", &saveptr);
        }
        
        pthread_mutex_unlock(&game_lock);
    }
    
    return NULL;
}

// ==================== HANDLE EVENTS ====================
void handle_events() {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) {
            game.running = 0;
        }
        
        pthread_mutex_lock(&game_lock);
        
        if(e.type == SDL_MOUSEBUTTONDOWN) {
            int x = e.button.x;
            int y = e.button.y;
            
            if(game.state == STATE_LOGIN) {
                login_screen_handle_click(&game, x, y);
                
                // Send login/register command
                if(x >= 350 && x <= 500 && y >= 400 && y <= 450) {
                    if(strlen(game.username_field.text) > 0 && strlen(game.password_field.text) > 0) {
                        char msg[256];
                        if(game.is_register_mode) {
                            snprintf(msg, sizeof(msg), "REGISTER:%s:%s", 
                                     game.username_field.text, game.password_field.text);
                        } else {
                            snprintf(msg, sizeof(msg), "LOGIN:%s:%s", 
                                     game.username_field.text, game.password_field.text);
                        }
                        send_msg(msg);
                        strcpy(game.login_message, "Processing...");
                    }
                }
            }
            else if(game.state == STATE_PROFILE) {
                profile_screen_handle_click(&game, x, y);
            }
            else if(game.state == STATE_MATCHMAKING) {
                matchmaking_screen_handle_click(&game, x, y);
            }
            else if(game.state == STATE_MATCH_FOUND) {
                match_found_screen_handle_click(x, y, &game);
            }
            else if(game.state == STATE_LOBBY) {
                lobby_screen_handle_click(&game, x, y);
                
                // Send commands for buttons only (INVITE is sent in lobby_screen_handle_click)
                if(x >= 50 && x <= 200 && y >= 620 && y <= 670) {
                    send_msg("GET_USERS");
                }
                if(x >= 220 && x <= 370 && y >= 620 && y <= 670) {
                    send_msg("LOGOUT");
                }
            }
            else if(game.state == STATE_SENDING_INVITE || game.state == STATE_WAITING_INVITE || game.state == STATE_RECEIVED_INVITE) {
                invitation_screen_handle_click(&game, x, y);
            }
            else if(game.state == STATE_LOBBY && strlen(game.message) > 0 && strstr(game.message, "declined") != NULL) {
                // Handle decline notification dialog
                invitation_screen_handle_click(&game, x, y);
            }
            else if(game.state == STATE_PLACING_SHIPS) {
                placing_ships_handle_click(&game, x, y);
                
                // PLACE command is handled in placing_ships_handle_click
                // which updates the map locally, then we send to server
                
                // Send READY command
                int ready_x = 200 + 150, ready_y = 100 + MAP_SIZE * 28 + 20;
                if(x >= ready_x && x <= ready_x + 150 && y >= ready_y && y <= ready_y + 50) {
                    if(placing_ships_check_complete(&game)) {
                        send_msg("READY");
                    }
                }
            }
            else if(game.state == STATE_PLAYING) {
                playing_screen_handle_click(&game, x, y);
                
                // Send FIRE command
                if(game.is_my_turn) {
                    int start_x = 530, start_y = 100;
                    if(x >= start_x && x < start_x + MAP_SIZE * CELL_SIZE &&
                       y >= start_y && y < start_y + MAP_SIZE * CELL_SIZE) {
                        int col = (x - start_x) / CELL_SIZE;
                        int row = (y - start_y) / CELL_SIZE;
                        if(col >= 0 && col < MAP_SIZE && row >= 0 && row < MAP_SIZE) {
                            char cmd[64];
                            snprintf(cmd, sizeof(cmd), "FIRE:%d,%d", col + 1, row + 1);
                            send_msg(cmd);
                        }
                    }
                }
            }
            else if(game.state == STATE_GAME_OVER) {
                game_over_screen_handle_click(&game, x, y);
            }
        }
        
        if(e.type == SDL_TEXTINPUT) {
            if(game.state == STATE_LOGIN) {
                login_screen_handle_text(&game, e.text.text);
            } else if(game.state == STATE_LOBBY && game.player_search_field.is_active) {
                // Handle search box text input
                int len = strlen(game.player_search_field.text);
                if(len < 49) {  // Leave room for null terminator
                    strcat(game.player_search_field.text, e.text.text);
                }
            }
        }

        if(e.type == SDL_KEYDOWN) {
            if(game.state == STATE_LOGIN) {
                login_screen_handle_key(&game, e.key.keysym.sym);
            } else if(game.state == STATE_LOBBY && game.player_search_field.is_active) {
                // Handle backspace in search box
                if(e.key.keysym.sym == SDLK_BACKSPACE) {
                    int len = strlen(game.player_search_field.text);
                    if(len > 0) {
                        game.player_search_field.text[len - 1] = '\0';
                    }
                } else if(e.key.keysym.sym == SDLK_ESCAPE) {
                    // Clear search and deactivate
                    game.player_search_field.text[0] = '\0';
                    game.player_search_field.is_active = 0;
                    SDL_StopTextInput();
                }
            } else if(game.state == STATE_PLACING_SHIPS) {
                placing_ships_handle_key(&game, e.key.keysym.sym);
                if(e.key.keysym.sym == SDLK_q) {
                    game.state = STATE_LOBBY;
                    send_msg("GET_USERS");
                }
            } else if(game.state == STATE_PLAYING) {
                if(e.key.keysym.sym == SDLK_q) {
                    game.state = STATE_LOBBY;
                    send_msg("GET_USERS");
                }
            }
        }
        
        if(e.type == SDL_MOUSEMOTION && game.state == STATE_PLACING_SHIPS) {
            placing_ships_handle_motion(&game, e.motion.x, e.motion.y);
        }
        
        pthread_mutex_unlock(&game_lock);
    }
}

// ==================== RENDER ====================
void render() {
    pthread_mutex_lock(&game_lock);
    
    if(game.state == STATE_LOGIN) {
        login_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_LOBBY) {
        lobby_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_PROFILE) {
        profile_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_MATCHMAKING) {
        matchmaking_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_MATCH_FOUND) {
        match_found_screen_render(game.renderer, game.font, &game);
    }
    else if(game.state == STATE_SENDING_INVITE || game.state == STATE_WAITING_INVITE || game.state == STATE_RECEIVED_INVITE) {
        lobby_screen_render(game.renderer, &game);
        invitation_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_LOBBY && strlen(game.message) > 0 && strstr(game.message, "declined") != NULL) {
        lobby_screen_render(game.renderer, &game);
        invitation_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_PLACING_SHIPS) {
        placing_ships_render(game.renderer, &game);
    }
    else if(game.state == STATE_PLAYING) {
        playing_screen_render(game.renderer, &game);
    }
    else if(game.state == STATE_GAME_OVER) {
        game_over_screen_render(game.renderer, &game);
    }
    
    pthread_mutex_unlock(&game_lock);
    SDL_RenderPresent(game.renderer);
}

// ==================== MAIN ====================
int main(int argc, char* argv[]) {
    // Parse command-line arguments for server IP
    if(argc >= 2) {
        server_ip = argv[1];
        printf("Server IP from command-line: %s\n", server_ip);
    } else {
        printf("Using default server IP: %s\n", server_ip);
        printf("Usage: %s [server_ip]\n", argv[0]);
        printf("Example: %s 192.168.1.100\n", argv[0]);
    }

    init_game();

    // Create receive thread
    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);
    pthread_detach(tid);

    GameState prev_state = STATE_LOGIN;

    while(game.running) {
        handle_events();

        unsigned int current_time = SDL_GetTicks();

        // Check for state changes
        pthread_mutex_lock(&game_lock);
        if(game.state == STATE_LOBBY && prev_state != STATE_LOBBY) {
            // Just entered lobby - request updated stats, users and leaderboard
            send_msg("GET_MY_STATS");
            send_msg("GET_USERS");
            send_msg("GET_LEADERBOARD");
            game.last_leaderboard_update = current_time;
            game.last_users_update = current_time;
        }

        // Auto-refresh leaderboard every 10 seconds when in lobby
        if(game.state == STATE_LOBBY) {
            if(current_time - game.last_leaderboard_update >= 10000) {
                send_msg("GET_LEADERBOARD");
                game.last_leaderboard_update = current_time;
            }

            // Auto-refresh player list every 3.5 seconds
            if(current_time - game.last_users_update >= 3500) {
                send_msg("GET_USERS");
                game.last_users_update = current_time;
            }
        }

        // Handle logout request
        if(game.logout_requested) {
            send_msg("LOGOUT");
            game.logout_requested = 0;
        }

        prev_state = game.state;
        pthread_mutex_unlock(&game_lock);

        render();
        SDL_Delay(16); // ~60 FPS
    }
    
    close(game.sockfd);

    // Giải phóng assets
    assets_cleanup(&game.assets);

    TTF_CloseFont(game.font);
    TTF_CloseFont(game.font_small);
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}


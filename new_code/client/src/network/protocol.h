// protocol.h - Protocol parsing functions
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../core/game_data.h"

// Parse STATE message from server
int parse_state_message(GameData* game, const char* state_data);

// Parse server messages (LOGIN_OK, USERS, etc.)
int parse_server_message(GameData* game, const char* msg);

// Parse login response
int parse_login_response(GameData* game, const char* msg);

// Parse user list
int parse_users_list(GameData* game, const char* msg);

#endif



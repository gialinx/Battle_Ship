CC = gcc
CFLAGS = -Wall -Wextra -pthread
SDL_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image -lm
DB_FLAGS = -lsqlite3

SERVER = server
SERVER_LOBBY = server_lobby
CLIENT = client
CLIENT_GUI = client_gui
CLIENT_GUI_LOGIN = client_gui_login

all: $(SERVER) $(SERVER_LOBBY) $(CLIENT) $(CLIENT_GUI) $(CLIENT_GUI_LOGIN)

$(SERVER): server.c src/database.c
	$(CC) $(CFLAGS) server.c src/database.c -o $(SERVER) $(DB_FLAGS) -lm

$(SERVER_LOBBY): src/server_lobby.c src/database.c
	$(CC) $(CFLAGS) src/server_lobby.c src/database.c -o $(SERVER_LOBBY) $(DB_FLAGS) -lm

$(CLIENT): client.c
	$(CC) $(CFLAGS) client.c -o $(CLIENT)

$(CLIENT_GUI): src/client_gui_v2.c src/network.c src/database.c
	$(CC) $(CFLAGS) src/client_gui_v2.c src/network.c src/database.c -o $(CLIENT_GUI) $(SDL_FLAGS) $(DB_FLAGS)

$(CLIENT_GUI_LOGIN): src/client_gui_login.c
	$(CC) $(CFLAGS) src/client_gui_login.c -o $(CLIENT_GUI_LOGIN) $(SDL_FLAGS)

clean:
	rm -f $(SERVER) $(SERVER_LOBBY) $(CLIENT) $(CLIENT_GUI) $(CLIENT_GUI_LOGIN) battleship.db

re: clean all
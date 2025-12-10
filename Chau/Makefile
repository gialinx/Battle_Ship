CC = gcc
CFLAGS = -Wall -Wextra -pthread
SDL_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image -lm

SERVER = server
CLIENT = client
CLIENT_GUI = client_gui

all: $(SERVER) $(CLIENT) $(CLIENT_GUI)

$(SERVER): server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER)

$(CLIENT): client.c
	$(CC) $(CFLAGS) client.c -o $(CLIENT)

$(CLIENT_GUI): src/client_gui_v2.c src/network.c
	$(CC) $(CFLAGS) src/client_gui_v2.c src/network.c -o $(CLIENT_GUI) $(SDL_FLAGS)

clean:
	rm -f $(SERVER) $(CLIENT) $(CLIENT_GUI)

re: clean all

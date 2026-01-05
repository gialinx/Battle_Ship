# Makefile cho Battle Ship Client
CC = gcc
CFLAGS = -Wall -Wextra -pthread
SDL_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lm

# Output binary
CLIENT = client_gui

all: $(CLIENT)

$(CLIENT): src/client/client_gui_complete.c \
           src/ui/renderer.c \
           src/ui/screens/login_screen.c \
           src/ui/screens/lobby_screen.c \
           src/ui/screens/profile_screen.c \
           src/ui/screens/matchmaking_screen.c \
           src/ui/screens/match_found_screen.c \
           src/ui/screens/invitation_screen.c \
           src/ui/screens/placing_ships_screen.c \
           src/ui/screens/playing_screen.c \
           src/ui/screens/game_over_screen.c \
           src/network/protocol.c \
           src/network/network.c \
           src/core/assets.c
	$(CC) $(CFLAGS) -Isrc src/client/client_gui_complete.c \
		src/ui/renderer.c \
		src/ui/screens/login_screen.c \
		src/ui/screens/lobby_screen.c \
		src/ui/screens/profile_screen.c \
		src/ui/screens/matchmaking_screen.c \
		src/ui/screens/match_found_screen.c \
		src/ui/screens/invitation_screen.c \
		src/ui/screens/placing_ships_screen.c \
		src/ui/screens/playing_screen.c \
		src/ui/screens/game_over_screen.c \
		src/network/protocol.c \
		src/network/network.c \
		src/core/assets.c \
		-o $(CLIENT) $(SDL_FLAGS)

clean:
	rm -f $(CLIENT)

.PHONY: all clean

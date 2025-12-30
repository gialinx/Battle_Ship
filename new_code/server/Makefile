# Makefile cho Battle Ship Server
CC = gcc
CFLAGS = -Wall -Wextra -pthread
DB_FLAGS = -lsqlite3 -lm

# Output binaries
SERVER = server
SERVER_LOBBY = server_lobby

all: $(SERVER) $(SERVER_LOBBY)

$(SERVER): server.c src/database.c src/matchmaking.c
	$(CC) $(CFLAGS) server.c src/database.c src/matchmaking.c -o $(SERVER) $(DB_FLAGS)

$(SERVER_LOBBY): src/server_lobby.c src/database.c src/matchmaking.c
	$(CC) $(CFLAGS) src/server_lobby.c src/database.c src/matchmaking.c -o $(SERVER_LOBBY) $(DB_FLAGS)

clean:
	rm -f $(SERVER) $(SERVER_LOBBY)

# Target riêng để xóa database (CHỈ dùng khi muốn reset)
clean-db:
	rm -f battleship.db
	@echo "⚠️  Database đã bị xóa!"

# Xóa tất cả (bao gồm database)
clean-all: clean clean-db

.PHONY: all clean

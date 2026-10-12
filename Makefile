
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g -pthread

# -lrt pre zdieľanú pamäť
# -lpthread pre vlákna
# -lm pre matematické funkcie
LDFLAGS = -lrt -lpthread -lm


SERVER_BIN = bin/server
CLIENT_BIN = bin/client


COMMON_SRC = src/mapa.c src/chodec.c src/simulacia.c

SERVER_SRC = src/server.c

CLIENT_SRC = src/klient.c src/main.c src/inputHandler.c


COMMON_OBJ = $(COMMON_SRC:src/%.c=obj/%.o)
SERVER_OBJ = $(SERVER_SRC:src/%.c=obj/%.o)
CLIENT_OBJ = $(CLIENT_SRC:src/%.c=obj/%.o)



all: server client


server: $(SERVER_BIN)


client: $(CLIENT_BIN)


$(SERVER_BIN): $(SERVER_OBJ) $(COMMON_OBJ)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)


$(CLIENT_BIN): $(CLIENT_OBJ) $(COMMON_OBJ)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)


obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf obj bin
	rm -f /dev/shm/nahodna_pochodzka_shm
	@echo "Projekt bol vyčistený."


.PHONY: all clean server client
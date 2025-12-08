CC = gcc
CFLAGS = -Wall -Iinclude
SRC = src/http.c src/server.c src/backend.c src/main.c src/logging.c
OUT = build/server

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -rf build
	rm -f server.log

debug: CFLAGS += -DDEBUG -g
debug: all

release: CFLAGS += -O2
release: all

.PHONY: all run clean debug release
CC = gcc
CFLAGS = -Wall -Iinclude
SRC = src/http.c src/server.c src/main.c
OUT = build/server

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -rf build


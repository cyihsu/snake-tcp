all:
	gcc client.c -o client -lncurses -lpthread -O3 -std=c99
	gcc server.c -o server -lpthread -O3 -std=c99
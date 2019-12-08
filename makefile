all:
	gcc client.c -o client -lncurses -lpthread -O3
	gcc server.c -o server -lpthread -O3
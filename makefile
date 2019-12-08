all:
	gcc client.c -o client -lncurses -lpthread
	gcc server.c -o server
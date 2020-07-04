CC = gcc -Wall -g 

all:  client server

client: client.c client.h
		$(CC) -o client client.c


server: main_server.c main_server.h
		$(CC) -o server main_server.c

#gcc client.c client.h -o client
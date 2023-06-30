CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter

all: cliente chatroom

cliente: cliente.c cliente.h
	$(CC) $(CFLAGS) -o cliente cliente.c

chatroom: chatroom.c cliente.h
	$(CC) $(CFLAGS) -o chatroom chatroom.c

clean:
	rm -f cliente chatroom

run:
	make clean
	make
	./chatroom 127.0.0.1 8080
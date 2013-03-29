all: chat
chat: chat.o
	gcc -D chat chat.c -o chat1

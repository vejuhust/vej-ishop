all: little cgi mat

CC = gcc

CFLAGS = -O3

little : little.c
	$(CC) little.c -o little $(CFLAGS)

cgi :
	(cd cgi-bin; make)

mat :
	(cd matrix; make)


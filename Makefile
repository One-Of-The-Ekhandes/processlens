		CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11

all: processlens		processlens: src/processlens.c
	$(CC) $(CFLAGS) -o processlens src/processlens.c		clean:
	rm -f processlens

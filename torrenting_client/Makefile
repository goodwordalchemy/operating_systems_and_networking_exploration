CC = gcc
CFLAGS=-Wall -g

all: urtorrent

urtorrent.o: urtorrent.c
	$(CC) $(CFLAGS) -c urtorrent.c

urtorrent: urtorrent.o
	$(CC) $(CFLAGS) -o urtorrent urtorrent.o

.PHONY: clean

clean: 
	rm *.o

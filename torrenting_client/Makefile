CC = gcc
CFLAGS=-Wall -g

all: urtorrent

urtorrent.o: urtorrent.c
	$(CC) $(CFLAGS) -c urtorrent.c

bencode.o: bencode/bencode.c bencode/bencode.h
	$(CC) $(CFLAGS) -c bencode/bencode.c

urtorrent: urtorrent.o bencode.o
	$(CC) $(CFLAGS) -o urtorrent urtorrent.o bencode.o

.PHONY: clean

clean: 
	rm *.o

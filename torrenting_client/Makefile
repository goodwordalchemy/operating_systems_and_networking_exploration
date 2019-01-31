CC = gcc
CFLAGS=-Wall -g 

all: urtorrent

urtorrent.o: urtorrent.c
	$(CC) $(CFLAGS) -c urtorrent.c

bencode.o: bencode/bencode.c bencode/bencode.h
	$(CC) $(CFLAGS) -c bencode/bencode.c

metainfo.o: metainfo.c metainfo.h bencode.o
	$(CC) $(CFLAGS) -c -lcrypto metainfo.c

urtorrent: urtorrent.o bencode.o metainfo.c
	$(CC) $(CFLAGS) -o urtorrent urtorrent.o bencode.o metainfo.c

.PHONY: clean

clean: 
	rm *.o

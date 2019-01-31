CC = gcc
CFLAGS=-Wall -g 

all: urtorrent

urtorrent.o: urtorrent.c
	$(CC) $(CFLAGS) -c urtorrent.c

bencode.o: bencode/bencode.c bencode/bencode.h
	$(CC) $(CFLAGS) -c bencode/bencode.c

filestring.o: filestring.c filestring.h
	$(CC) $(CFLAGS) -c filestring.c

metainfo.o: metainfo.c metainfo.h bencode.o filestring.o
	$(CC) $(CFLAGS) -c -lcrypto metainfo.c

urtorrent: urtorrent.o bencode.o metainfo.c filestring.o
	$(CC) $(CFLAGS) -o urtorrent urtorrent.o bencode.o metainfo.c filestring.o

.PHONY: clean

clean: 
	rm *.o

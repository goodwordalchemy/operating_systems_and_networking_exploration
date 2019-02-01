CC = gcc
CFLAGS=-Wall -g 
LDFLAGS=-L/usr/local/Cellar//openssl@1.1/1.1.0g/lib/\
		-I/usr/local/Cellar//openssl@1.1/1.1.0g/

all: urtorrent

urtorrent.o: urtorrent.c
	$(CC) $(CFLAGS) -c urtorrent.c

bencode.o: bencode/bencode.c bencode/bencode.h
	$(CC) $(CFLAGS) -c bencode/bencode.c

filestring.o: filestring.c filestring.h
	$(CC) $(CFLAGS) -c filestring.c

metainfo.o: metainfo.c metainfo.h
	$(CC) $(CFLAGS) -c metainfo.c

ip_address.o: ip_address.c ip_address.h
	$(CC) $(CFLAGS) -c ip_address.c

urtorrent: urtorrent.o\
   			bencode.o\
   			filestring.o\
   			metainfo.o\
   			ip_address.o
	$(CC) $(CFLAGS) $(LDFLAGS) -lcrypto -o urtorrent urtorrent.o\
	   	bencode.o\
	   	filestring.o\
	   	metainfo.o\
	   	ip_address.o

.PHONY: clean

clean: 
	rm *.o

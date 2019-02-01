# A note on macros:
# 	% is like a wildcard.
# 	$@ refers to the left side of the colon
# 	$< is the first item in the dependecy list
# 	$^ is the right side of the colon
CC = gcc
CFLAGS=-Wall -g 
LDFLAGS=-L/usr/local/Cellar//openssl@1.1/1.1.0g/lib/\
		-I/usr/local/Cellar//openssl@1.1/1.1.0g/

DEPS=filestring.h\
	 ip_address.h\
	 metainfo.h\
	 socket_helpers.h

OBJS=urtorrent.o\
	 bencode/bencode.o\
	 filestring.o\
	 metainfo.o\
	 ip_address.o


all: urtorrent

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

urtorrent: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lcrypto -o urtorrent $(OBJS)

.PHONY: clean

clean: 
	rm *.o

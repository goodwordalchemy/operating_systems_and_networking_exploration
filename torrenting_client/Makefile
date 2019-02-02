# A note on macros:
# 	% is like a wildcard.
# 	$@ refers to the left side of the colon
# 	$< is the first item in the dependecy list
# 	$^ is the right side of the colon
CC = gcc
CFLAGS=-Wall -g 
LDFLAGS=-L/usr/local/Cellar//openssl@1.1/1.1.0g/lib/\
		-I/usr/local/Cellar//openssl@1.1/1.1.0g/

DEPS=announce.h\
	 bencode/bencode.h\
	 be_node_utils.h\
	 filestring.h\
	 http_response_parse.h\
	 ip_address.h\
	 metainfo.h\
	 repl.h\
	 socket_helpers.h\
	 state.h\
	 url_parse.h\

OBJS=announce.o\
	 bencode/bencode.o\
	 be_node_utils.o\
	 filestring.o\
	 http_response_parse.o\
	 ip_address.o\
	 metainfo.o\
	 repl.o\
	 socket_helpers.o\
	 url_parse.o\
	 urtorrent.o\

all: urtorrent

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

urtorrent: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lcrypto -o urtorrent $(OBJS)

.PHONY: clean

clean: 
	rm *.o

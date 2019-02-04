# A note on macros:
# 	% is like a wildcard.
# 	$@ refers to the left side of the colon
# 	$< is the first item in the dependecy list
# 	$^ is the right side of the colon
SRCDIR=src
OBJDIR=obj
CC = gcc
CFLAGS=-Wall -g 
LDFLAGS=-L/usr/local/Cellar//openssl@1.1/1.1.0g/lib/\
		-I/usr/local/Cellar//openssl@1.1/1.1.0g/

_DEPS=announce.h\
	 bencode/bencode.h\
	 be_node_utils.h\
	 filestring.h\
	 handshake.h\
	 http_response_parse.h\
	 ip_address.h\
	 logging_utils.h\
	 metainfo.h\
	 messages.h\
	 pieces.h\
	 socket_helpers.h\
	 state.h
DEPS=$(patsubst %, $(SRCDIR)/%, $(_DEPS))

_OBJS=announce.o\
	 bencode.o\
	 be_node_utils.o\
	 filestring.o\
	 handshake.o\
	 http_response_parse.o\
	 logging_utils.o\
	 ip_address.o\
	 metainfo.o\
	 messages.o\
	 pieces.o\
	 socket_helpers.o\
	 urtorrent.o
OBJS=$(patsubst %, $(OBJDIR)/%, $(_OBJS))

all: urtorrent

$(OBJDIR)/bencode.o: $(SRCDIR)/bencode/bencode.c $(SRCDIR)/bencode/bencode.h
	$(CC) -c -o $@ $< $(CFLAGS)
	

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

urtorrent: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lcrypto -o $@ $^

.PHONY: clean

clean: 
	rm $(OBJDIR)/*.o
	rm -rf *.dSYM

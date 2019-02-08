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

_DEPS=bencode/bencode.h\
	  bitfield.h\
	  be_node_utils.h\
	  encoding_utils.h\
	  file_utils.h\
	  piece_trading.h\
	  hash_utils.h\
	  ip_address.h\
	  logging_utils.h\
	  metainfo.h\
	  messages.h\
	  pieces.h\
	  socket_helpers.h\
	  state.h\
	  tracker.h
DEPS=$(patsubst %, $(SRCDIR)/%, $(_DEPS))

_OBJS=bencode.o\
	  bitfield.o\
	  be_node_utils.o\
	  encoding_utils.o\
	  file_utils.o\
	  piece_trading.o\
	  hash_utils.o\
	  logging_utils.o\
	  ip_address.o\
	  metainfo.o\
	  messages.o\
	  pieces.o\
	  socket_helpers.o\
	  tracker.o\
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

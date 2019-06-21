# Makefile for 'player, server, and grid'
# Austin Zhang, May 2019
# Edited by Sunbir Chawla
#  and Aadil Islam


M = support

PROG = server
OBJS = server.o grid.o
PROG2 = player
OBJS2 = player.o

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$M
CC = gcc
MAKE = make
LIBS = -lm -lncurses
LLIBS = $M/support.a

.PHONY: clean

all: $(PROG) $(PROG2)

$(PROG): $(OBJS) $(LLIBS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

$(PROG2): $(OBJS2) $(LLIBS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

server.o: $M/memory.h $M/message.h $M/log.h grid.h

grid.o: $M/memory.h $M/log.h grid.h

player.o: $M/message.h $M/log.h 

$(LLIBS):
	make -C $M support.a
	
clean:
	make -C $M clean
	rm -f *log
	rm -f *~ *.o
	rm -f $(PROG) $(PROG2)
	rm -f core

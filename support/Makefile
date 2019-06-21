#
# Makefile for 'support.a'
# CS50 project Spring 2019
#
# Anything that links with support.a must also link with the math library.
#    gcc ... support.a -lm ...
#
# David Kotz, May 2019
#

LIB = support.a
TESTS = messagetest

CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make

.PHONY: all clean

############# default rule ###########
all: $(LIB) $(TESTS) 

$(LIB): message.o log.o memory.o
	ar cr $(LIB) $^

messagetest: message.c message.h log.o file.o 
	$(CC) $(CFLAGS) -DUNIT_TEST message.c log.o file.o -lm -o messagetest

message.o: message.h
log.o: log.h
file.o: file.h
memory.o: memory.h

############# clean ###########
clean:
	rm -f core
	rm -rf *~ *.o *.dSYM
	rm -f *.log
	rm -f $(LIB)
	rm -f $(TESTS)
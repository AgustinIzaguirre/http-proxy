CC 			= gcc
CFLAGS 		= -Wall -pedantic -std=c99 -I ./include -I ./../management-protocol/include -o httpdctl
CSOURCES  	= $(wildcard ./../management-protocol/*.c *.c)
LINKFLAGS 	= -L/usr/local/lib -lsctp

all:
	$(CC) $(CFLAGS) $(CSOURCES) $(LINKFLAGS)

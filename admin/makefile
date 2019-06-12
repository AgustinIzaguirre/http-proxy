CC 			= gcc
CFLAGS 		= -Wall -pedantic -std=c99 -I ./include -I ./../adminProtocol/include -o admin
CSOURCES  	= $(wildcard ./../adminProtocol/*.c *.c)
LINKFLAGS 	= -L/usr/local/lib -lsctp

all:
	$(CC) $(CFLAGS) $(CSOURCES) $(LINKFLAGS)

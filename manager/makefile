CC 			= gcc
CFLAGS 		= -Wall -pedantic -std=c99 -D_DEFAULT_SOURCE -I ./include -I ./../management-protocol/include -o manager
CSOURCES  	= $(wildcard ./../management-protocol/*.c *.c)
LINKFLAGS 	= -L/usr/local/lib -lsctp -D_POSIX_C_SOURCE=199309L

all:
	$(CC) $(CFLAGS) $(CSOURCES) $(LINKFLAGS)

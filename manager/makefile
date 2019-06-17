CC 			= gcc
CFLAGS 		= -Wall -pedantic -D_DEFAULT_SOURCE -std=c99 -I ./include -I ./../management-protocol/include -o httpdctl
CSOURCES  	= $(wildcard ./../management-protocol/*.c *.c)
LINKFLAGS 	= -L/usr/local/lib -lsctp -D_POSIX_C_SOURCE=199309L

all:
	$(CC) $(CFLAGS) $(CSOURCES) $(LINKFLAGS)

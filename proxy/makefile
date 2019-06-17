COMPILER   = gcc
CFLAGS     = -Wall -pedantic -g -o0 -D_DEFAULT_SOURCE -std=c99 -I ./include -I ./../management-protocol/include -I ./../logger/include
CSOURCES   = $(wildcard ./../management-protocol/*.c ./../logger/*.c *.c)
LINKFLAGS  = -L/usr/local/lib -lsctp -lpthread -D_POSIX_C_SOURCE=199309L

all:
	$(COMPILER) $(CFLAGS) $(CSOURCES) $(LINKFLAGS) -o httpd

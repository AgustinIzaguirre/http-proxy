COMPILER   = gcc
CFLAGS     = -Wall -pedantic -D_DEFAULT_SOURCE -std=c99 -I ./include
CSOURCES   = $(wildcard *.c)
LINKFLAGS  = -lpthread -D_POSIX_C_SOURCE=199309L

all:
	$(COMPILER) $(CFLAGS) $(CSOURCES) $(LINKFLAGS)

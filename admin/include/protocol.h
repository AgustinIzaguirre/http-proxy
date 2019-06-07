#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdlib.h>

#define AUTHENTICATION_STREAM 0

int establishConnection(char *serverIP, unsigned int serverPort);

#endif

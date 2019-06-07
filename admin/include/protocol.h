#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdlib.h>

void sendSCTPMsg(int server, void *msg, size_t msgLength, int streamNumber);

#endif

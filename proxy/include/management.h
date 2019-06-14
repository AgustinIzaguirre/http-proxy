#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <selector.h>
#include <sys/socket.h>
#include <sys/types.h>

#define STREAM_QUANTITY 2

const char *getManagementErrorMessage();

int listenManagementSocket(int managementSocket, size_t backlogQuantity);

void managementPassiveAccept(struct selector_key *key);

#endif

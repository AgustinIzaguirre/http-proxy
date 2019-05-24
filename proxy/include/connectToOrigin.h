#ifndef CONNECT_TO_ORIGIN_H
#define CONNECT_TO_ORIGIN_H

#include <selector.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

/*
 * Connects to origin server
 */
int connectToOrigin(struct selector_key *key, struct addrinfo *ipEntry);

int blockingToResolvName(struct selector_key *key, int fdClient);
void *addressResolvName(void **data);
unsigned addressResolvNameDone(struct selector_key *key);

#endif
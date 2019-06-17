#ifndef CONNECT_TO_ORIGIN_H
#define CONNECT_TO_ORIGIN_H

#include <selector.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <metric.h>

/*
 * Connects to origin server
 */
int connectToOrigin(struct selector_key *key, struct addrinfo *ipEntry);

/*
 * Create a thred that resolves the DNS query
 */
int blockingToResolvName(struct selector_key *key, int fdClient);

/*
 * Resolves the dns query
 */
void *addressResolvName(void **data);

/*
 * Iterates from the list of resolves and trys to connect to each
 */
unsigned addressResolvNameDone(struct selector_key *key);

#endif

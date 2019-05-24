#ifndef HTTP_H
#define HTTP_H

#include <selector.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

/*
 * Accepts a connection and creates a new struct http
 * associated with it.
 */
void httpPassiveAccept(struct selector_key *key);

/*
 * Function that takes place when clientFd is avaliable
 * to read on copy state
 */
unsigned parseMethodRead(struct selector_key *key);

/*
 * Initialize parse request struct
 */
void parseRequestInit(const unsigned state, struct selector_key *key);

/*
 * Destroys parse request struct
 */
void parseRequestDestroy(const unsigned state, struct selector_key *key);

unsigned parseTargetRead(struct selector_key *key);
void parseTargetArrive(const unsigned state, struct selector_key *key);
void parseTargetDeparture(const unsigned state,
						  struct selector_key *key); // TODO

/*
 * Connects to origin server
 */
int connectToOrigin(struct selector_key *key, struct addrinfo *ipEntry);

int blockingToResolvName(struct selector_key *key, int fdClient);
void *addressResolvName(void **data);
unsigned addressResolvNameDone(struct selector_key *key);

#endif

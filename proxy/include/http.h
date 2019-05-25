#ifndef HTTP_H
#define HTTP_H

#include <selector.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#define GET_DATA(key) ((httpADT_t)(key)->data)

/*
 * Returns the address of http handler structure
 */
const struct fd_handler *getHttpHandler();

/*
 * Accepts a connection and creates a new struct http
 * associated with it.
 */
void httpPassiveAccept(struct selector_key *key);

#endif

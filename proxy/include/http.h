#ifndef HTTP_H
#define HTTP_H

#include <selector.h>

/*
 * Accepts a connection and creates a new struct http
 * associated with it.
 */
void httpPassiveAccept(struct selector_key *key);

/*
 * Function that takes place when clientFd is avaliable
 * to read on copy state
 */
unsigned printRead(struct selector_key *key);

#endif
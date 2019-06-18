#ifndef HANDLE_PARSERS_H
#define HANDLE_PARSERS_H

#include <selector.h>
#include <httpProxyADT.h>

#define GET_DATA(key) ((httpADT_t)(key)->data)

/*
 * Reads from a socket and then it parses what it read.
 * Return the next state in the proxy state machine
 */
unsigned parseRead(struct selector_key *key);

/*
 * Initialize all the parsers
 */
void parseInit(const unsigned state, struct selector_key *key);

/*
 * Destroy all the parsers
 */
void parseDestroy(const unsigned state, struct selector_key *key);

#endif

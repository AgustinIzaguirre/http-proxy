#ifndef HANDLE_REQUEST_H
#define HANDLE_REQUEST_H

#include <selector.h>
/*
 * Reads request from client fd into readBuffer
 */
unsigned requestRead(struct selector_key *key);

/*
 * Writes from readBuffer to originFd
 */
unsigned requestWrite(struct selector_key *key);

/*
 * Set corresponding interests to client fd and origin fd
 */
unsigned setAdecuateFdInterests(struct selector_key *key);

/*
 * Returns response state based on transformations, if transformations
 * are enabled returns HANDLE_RESPONSE_WITH_TRANSFORMATION otherwise
 * returns HANDLE_RESPONSE
 */
unsigned getAdecuateResponseState(struct selector_key *key);

#endif
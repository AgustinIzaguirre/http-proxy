#ifndef HANDLE_REQUEST_H
#define HANDLE_REQUEST_H

#include <selector.h>
#include <headersParser.h>

struct handleRequest {
	struct headersParser parseHeaders;
	unsigned requestState;
};

enum requestState { FIRST_BUFFER = 0, LAST_BUFFER };

/*
 * Initialize headers parser struct
 */
void requestInit(const unsigned state, struct selector_key *key);

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

#endif

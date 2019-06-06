#ifndef HANDLE_RESPONSE_H
#define HANDLE_RESPONSE_H

#include <selector.h>
#include <headersParser.h>

struct handleResponse {
	struct headersParser parseHeaders;
	// other info
};

/*
 * Initialize headers parser struct
 */
void responseInit(const unsigned state, struct selector_key *key);

/*
 * Reads response from origin fd into writeBuffer
 */
unsigned responseRead(struct selector_key *key);

/*
 * Writes from writeBuffer to clientFd
 */
unsigned responseWrite(struct selector_key *key);

/*
 * Set corresponding interests to client fd and origin fd
 */
unsigned setResponseFdInterests(struct selector_key *key);

#endif
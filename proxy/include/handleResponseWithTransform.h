#ifndef HANDLE_RESPONSE_WITH_TRANSFORM_H
#define HANDLE_RESPONSE_WITH_TRANSFORM_H

#include <headersParser.h>

struct handleResponseWithTransform {
	struct headersParser parseHeaders;
	// other info
};

/*
 * Initialize handleResponseWithTransform struct
 */
void responseWithTransformInit(const unsigned state, struct selector_key *key);

/*
 * Sets interest to file descriptor when reading response with
 * transform
 */
unsigned setResponseWithTransformFdInterests(struct selector_key *key);

unsigned responseWithTransformRead(struct selector_key *key);
unsigned responseWithTransformWrite(struct selector_key *key);
#endif
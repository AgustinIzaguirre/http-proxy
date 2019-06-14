#ifndef HANDLE_ERROR_H
#define HANDLE_ERROR_H

#include <selector.h>
#include <httpProxyADT.h>
#include <string.h>

enum errorType {
	DEFAULT,
	METHOD_NOT_ALLOW,
	VERSION_NOT_SUPPORTED,
	NOT_FOUND_HOST,
	FAIL_TO_CONNECT
};

/*
 * initialize the message to be send to the client
 */
void errorInit(const unsigned state, struct selector_key *key);

/*
 * send error message to client
 */
unsigned errorHandleWrite(struct selector_key *key);

#endif

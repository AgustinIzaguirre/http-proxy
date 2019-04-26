#ifndef TARGET_PARSER_H
#define TARGET_PARSER_H

#include <stdint.h>
#include <buffer.h>
#include <stdlib.h>

enum targetState {
	START_T,
	A_AUTH_OR_A_SCHEMA,
	BAR_A_SCHEMA,
	D_BAR_A_SCHEMA,
	A_USERINFO,
	PORT,
	A_AUTH,
	END
};

struct targetParser {
	enum targetState state;
	int charactersRead;
	char *host;
	unsigned int sizeHost;
	char *target;
	unsigned int sizeTarget;
	int port;
};

#define BLOCK 10
#define PORT_DEFAULT 80

/*
 * Initialize parser
 */
void parseTargetInit(struct targetParser *parser);

/*
 * Parse the given input until there is nothing to read on
 * the input buffer or finds a space
 */
int parseTarget(struct targetParser *parser, buffer *input);

/*
 * Returns the current state of the target state machine
 */
unsigned getTargetState(struct targetParser *parser);

/*
 * Returns the host found by the state machine or null if not found
 */
char *getHost(struct targetParser *parser);

/*
 * Returns the target found by the state machine
 */
char *getTarget(struct targetParser *parser);

#endif

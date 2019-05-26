#ifndef METHOD_PARSER_H
#define METHOD_PARSER_H

/* Parser for request method, avaliable methods are GET, HEAD, POST,
** DELETE and can be extended in the future
*/

#include <stdint.h>
#include <buffer.h>

enum methodState {
	START,
	CR,
	G,
	GE,
	GET,
	H,
	HE,
	HEA,
	HEAD,
	P,
	PO,
	POS,
	POST,
	PU,
	PUT,
	D,
	DE,
	DEL,
	DELE,
	DELET,
	DELETE,
	DONE_METHOD_STATE,
	ERROR_METHOD_STATE,
};

enum methodType {
	GET_METHOD,
	HEAD_METHOD,
	POST_METHOD,
	PUT_METHOD,
	DELETE_METHOD,
	NO_METHOD,
};

struct methodParser {
	unsigned state;
	int charactersRead;
	unsigned method;
};

/*
 * Initialize parser
 */
void parseMethodInit(struct methodParser *parser);

/*
 * Parse a single letter into method state machine
 */
int parseMethodChar(struct methodParser *parser, char l);

/*
 * Returns the current state of the method state machine
 */
unsigned getState(struct methodParser *parser);

/*
 * Returns the method found by the state machine
 */
unsigned getMethod(struct methodParser *parser);

#endif

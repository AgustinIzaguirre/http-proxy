#ifndef HEADERS_PARSER_H
#define HEADERS_PARSER_H

#include <selector.h>
#include <stdint.h>
#include <buffer.h>

#define MAX_HEADER_LENGTH 128

struct headersParser {
	char currHeader[MAX_HEADER_LENGTH];
	int headerIndex;
	int state;
};

enum headersState {
	HEADERS_START,
	HEADER_NAME,
	HEADER_VALUE,
	HEADER_END,
	HEADER_DONE,
	BODY_START,
};

/*
 * Initialize headers parser structure
 */
void initializeHeaderParser(struct headersParser **header);
/*
 * Parse a char into the headers parser statemachine
 */
void parseHeadersByChar(char l, struct headersParser *header);

/*
 * Resets header parser structure to initial state and reset its index
 */
void resetHeaderParser(struct headersParser *header);

/*
 * Parse the chars in buffer from begining to end inclusive
 */
void parseHeaders(struct headersParser *header, buffer *input, int begining,
				  int end);

#endif
#ifndef HEADERS_PARSER_H
#define HEADERS_PARSER_H

#include <selector.h>
#include <stdint.h>
#include <buffer.h>

#define MAX_HOP_BY_HOP_HEADER_LENGTH 20
#define MAX_HEADER_LENGTH MAX_HOP_BY_HOP_HEADER_LENGTH + 128
#define MAX_TOTAL_HEADER_LENGTH MAX_HEADER_LENGTH + 1024

struct headersParser {
	char currHeader[MAX_HEADER_LENGTH];
	uint8_t headerBuf[MAX_HEADER_LENGTH];
	buffer headerBuffer;
	int headerIndex;
	int state;
	uint8_t censure;
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
 * Parse the chars in buffer from begining to end inclusive and returns
 * how many chars has parsed
 */
void parseHeaders(struct headersParser *header, buffer *input, int begining,
				  int end);

void headersParserInit(struct headersParser *header); // TODO

#endif
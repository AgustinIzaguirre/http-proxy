#ifndef HEADERS_PARSER_H
#define HEADERS_PARSER_H

#include <selector.h>
#include <stdint.h>
#include <buffer.h>
#include <mediaRange.h>

#define MAX_HOP_BY_HOP_HEADER_LENGTH 20
#define MAX_HEADER_LENGTH MAX_HOP_BY_HOP_HEADER_LENGTH + 128
#define MAX_MIME_HEADER 128
#define BLOCK 10
#define MAX_TOTAL_HEADER_LENGTH MAX_HEADER_LENGTH + 1024

struct headersParser {
	char currHeader[MAX_HEADER_LENGTH];
	uint8_t headerBuf[MAX_HEADER_LENGTH];
	uint8_t mimeValue[MAX_MIME_HEADER];
	uint8_t valueBuf[20 + 30 + 20 +					// TODO: check the 20 addes
					 MAX_HOP_BY_HOP_HEADER_LENGTH]; // TODO set length with BUFF
													// size from configuration
	buffer headerBuffer;
	buffer valueBuffer;
	int headerIndex;
	int mimeIndex;
	int valueIndex;
	int state;
	uint8_t censure;
	uint8_t isMime;

	MediaRangePtr_t mediaRange;
	int censureContent;
	buffer *requestLineBuffer;
	buffer *responseLineBuffer;
	uint8_t isRequest;
};

enum headersState {
	FIRST_LINE,
	HEADERS_START,
	HEADER_NAME,
	HEADER_VALUE,
	HEADER_END,
	HEADER_DONE,
	BODY_START,
};

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

void headersParserInit(struct headersParser *header, struct selector_key *key,
					   uint8_t isRequest); // TODO comment

/*
 * Adds header connection: close to either request or response
 */
void addLastHeaders(struct headersParser *header);

/*
 * Copies current headerBuf into headerBuffer
 */
void copyBuffer(struct headersParser *header);

/*
 * Resets value index if necessary
 */
void resetValueBuffer(struct headersParser *header);

#endif

#ifndef HEADERS_PARSER_H
#define HEADERS_PARSER_H

#include <selector.h>
#include <stdint.h>
#include <buffer.h>
#include <mediaRange.h>
#include <httpProxyADT.h>

#define MAX_HOP_BY_HOP_HEADER_LENGTH 20
#define MAX_HEADER_LENGTH MAX_HOP_BY_HOP_HEADER_LENGTH + 128
#define MAX_MIME_HEADER 128
#define CHUNKED_LENGTH 7
#define BLOCK 10
#define IDENTITY_LENGTH 8
#define MAX_TOTAL_HEADER_LENGTH MAX_HEADER_LENGTH + 1024

struct headersParser {
	char currHeader[MAX_HEADER_LENGTH];
	uint8_t headerBuf[MAX_HEADER_LENGTH];
	uint8_t mimeValue[MAX_MIME_HEADER];
	uint8_t valueBuf[BUFFER_SIZE + 30 + 20 + // TODO: check the 20 addes
					 MAX_HOP_BY_HOP_HEADER_LENGTH];
	uint8_t transferValue[CHUNKED_LENGTH + 1];
	uint8_t contentValue[IDENTITY_LENGTH + 1];

	buffer headerBuffer;
	buffer valueBuffer;
	int headerIndex;
	int mimeIndex;
	int valueIndex;
	int state;
	uint8_t censure;
	uint8_t isMime;
	uint8_t isTransfer;
	uint8_t isChunked;
	uint8_t isChar;
	uint8_t isEncoded;
	uint8_t willTransform;
	uint8_t hasEncode;
	int tranferIndex;
	int contentIndex;
	int firstLine;

	MediaRangePtr_t mediaRange;
	int mediaRangeCurrent;
	int transformContent;
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

#define IS_100 -1
#define IS_NOT_100 -2

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

/*
 * Initialize headers parser structure with default values and initialize its
 * buffers
 */
void headersParserInit(struct headersParser *header, struct selector_key *key,
					   uint8_t isRequest);

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

/*
 * Returns transform content value from headers parser structure
 */
int getTransformContentParser(struct headersParser *header);

/*
 * Compares read value with chunked and if it is chunked it sets isChunked to
 * TRUE
 */
void compareWithChunked(struct headersParser *header);

/*
 * Compares read value with identity and if it is not identity it sets isEncoded
 * to TRUE
 */
void compareWithIdentity(struct headersParser *header);

/*
 * Returns true if will transform given encode and false if wont transform
 */
uint8_t getTransformEncode(struct headersParser *header);

/*
 * Handles headersParser FIRST_LINE state
 */
void handleFirstLine(char l, struct headersParser *header);

/*
 * Handles headersParser HEADERS_START state
 */
void handleHeadersStart(char l, struct headersParser *header);

/*
 * Handles headersParser HEADERS_NAME state
 */
void handleHeadersName(char l, struct headersParser *header);

/*
 * Handles headersParser HEADERS_VALUE state
 */
void handleHeaderValue(char l, struct headersParser *header);

/*
 * Handles headersParser HEADERS_END state
 */
void handleHeaderEnd(char l, struct headersParser *header);
#endif

#ifndef UNCHUNKPARSER_H
#define UNCHUNKPARSER_H

#include <stdint.h>
#include <httpProxyADT.h>

#define MAX_CHUNKED_ACCEPTED_BYTES                                             \
	16 // ULLONG_MAX expressed in hexa 0xFFFFFFFFFFFFFFFF

enum headersState {
	CHUNKED_SIZE,
	CHUNK_DATA,
	CHUNKED_ERROR,
};

struct unchunkParser {
	uint8_t unChunkedBufferData[BUFFER_SIZE];
	uint8_t chunkedBytes[MAX_CHUNKED_ACCEPTED_BYTES + 1];
	unsigned long long bytes;
	buffer unchunkedBuffer;
	int chunkedBytesIndex;
	uint8_t bytesFinished;
	unsigned state;
};

/*
 * Initialize unchunkParserStructure
 */
void unchunkParserInit(struct unchunkParser *unchunkParser,
					   struct selector_key *key);

/*
 * Parse buffer input and stores data as it wasn't chunked on unchunkedBuffer
 */
void parseChunkedInfo(struct unchunkParser *unchunkParser, buffer *input);

/*
 * Handle unchunkParser state machine
 */
void parseChunkedInfoByChar(uint8_t l, struct unchunkParser *unchunkParser);

/*
 * Sets bytes read in hexa in chunkedBytes
 */
void setBytes(struct unchunkParser *unchunkParser);

#endif

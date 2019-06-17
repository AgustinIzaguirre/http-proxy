#include <stdint.h>
#include <buffer.h>
#include <limits.h>
#include <unchunkParser.h>

#define ISHEXA(x)                                                              \
	(((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') ||               \
	 ((x) >= 'A' && (x) <= 'F'))

void unchunkParserInit(struct unchunkParser *unchunkParser,
					   struct selector_key *key) {
	memset(unchunkParser->unChunkedBufferData, 0, BUFFER_SIZE);
	buffer_init(&(unchunkParser->unchunkedBuffer), BUFFER_SIZE,
				unchunkParser->unChunkedBufferData);
	unchunkParser->state			 = CHUNKED_SIZE;
	unchunkParser->chunkedBytesIndex = 0;
	unchunkParser->bytesFinished	 = FALSE;
}

void parseChunkedInfo(struct unchunkParser *unchunkParser, buffer *input) {
	uint8_t l = buffer_read(input);

	while (l) {
		parseChunkedInfoByChar(l, unchunkParser);
		l = buffer_read(input);
	}
}

void parseChunkedInfoByChar(uint8_t l, struct unchunkParser *unchunkParser) {
	int state = unchunkParser->state;
	switch (state) {
		case CHUNKED_SIZE:
			if (unchunkParser->chunkedBytesIndex ==
				MAX_CHUNKED_ACCEPTED_BYTES) {
				unchunkParser->state = CHUNKED_ERROR;
			}
			else if (ISHEXA(l)) {
				unchunkParser
					->chunkedBytes[unchunkParser->chunkedBytesIndex++] = l;
			}
			else {
				if (!unchunkParser->bytesFinished) {
					unchunkParser
						->chunkedBytes[unchunkParser->chunkedBytesIndex] = 0;
					setBytes(unchunkParser);
					unchunkParser->bytesFinished = TRUE;
				}
				if (l == '\n') {
					unchunkParser->state = CHUNK_DATA;
				}
			}
			break;

		case CHUNK_DATA:
			if (unchunkParser->bytes > 0) {
				buffer_write(&unchunkParser->unchunkedBuffer, l);
			}
			else if (l == '\n') {
				unchunkParser->state = CHUNKED_SIZE;
			}
			break;
	}
}

void setBytes(struct unchunkParser *unchunkParser) {
	unchunkParser->bytes = hexaToULLong((char *) unchunkParser->chunkedBytes,
										unchunkParser->chunkedBytesIndex - 1);
}
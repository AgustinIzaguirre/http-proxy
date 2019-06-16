#include <stdio.h>
#include <stdlib.h>
#include <headersParser.h>
#include <configuration.h>
#include <utilities.h>
#include <string.h>
#include <ctype.h>
#include <http.h>
#include <httpProxyADT.h>

#define isOWS(a) (a == ' ' || a == '\t')

static void isCensureHeader(struct headersParser *header);

void headersParserInit(struct headersParser *header, struct selector_key *key,
					   uint8_t isRequest) {
	header->state			   = FIRST_LINE;
	header->headerIndex		   = 0;
	header->valueIndex		   = 0;
	header->mimeIndex		   = 0;
	header->censure			   = FALSE;
	header->isMime			   = FALSE;
	header->isTransfer		   = FALSE;
	header->isChunked		   = FALSE;
	header->requestLineBuffer  = getRequestLineBuffer(GET_DATA(key));
	header->responseLineBuffer = getResponseLineBuffer(GET_DATA(key));
	header->isRequest		   = isRequest;
	header->transformContent   = FALSE;
	header->mediaRangeCurrent  = 0;
	header->mediaRange		   = getMediaRangeHTTP(GET_DATA(key));
	header->tranferIndex	   = 0;
	header->contentIndex	   = 0;
	header->isEncoded		   = FALSE;
	header->hasEncode		   = FALSE;
	header->isChar			   = FALSE;
	header->willTransform	  = FALSE;

	memset(header->mimeValue, 0, MAX_MIME_HEADER);
	memset(header->transferValue, 0, CHUNKED_LENGTH + 1);
	memset(header->contentValue, 0, IDENTITY_LENGTH + 1);

	buffer_init(&(header->headerBuffer), MAX_HEADER_LENGTH, header->headerBuf);
	buffer_init(&(header->valueBuffer),
				BUFFER_SIZE + 30 + 20 + MAX_HOP_BY_HOP_HEADER_LENGTH,
				header->valueBuf);
}

void parseHeaders(struct headersParser *header, buffer *input, int begining,
				  int end) {
	uint8_t l = buffer_read(input);
	size_t spaceLeft;

	while (l) {
		parseHeadersByChar(l, header);
		if (header->state == HEADER_DONE) {
			printf("%s\n", header->currHeader);
			resetHeaderParser(header);
		}
		else if (header->state == BODY_START) {
			addLastHeaders(header);
			printf("body start\n");
			return;
		}
		buffer_write_ptr(&header->valueBuffer, &spaceLeft);
		if (spaceLeft <= MAX_HOP_BY_HOP_HEADER_LENGTH) {
			return;
		}
		l = buffer_read(input);
	}
}

void parseHeadersByChar(char l, struct headersParser *header) {
	int state = header->state;
	switch (state) {
		case FIRST_LINE:
			if (l == '\n') {
				header->state = HEADERS_START;
			}
			else if (header->isRequest) {
				buffer_write(header->requestLineBuffer, l);
			}
			else {
				buffer_write(header->responseLineBuffer, l);
			}
			buffer_write(&header->valueBuffer, l);
			break;
		case HEADERS_START:
			if (l == '\n') {
				header->state = BODY_START;
			}
			else if (l == '\r') {
				header->state = HEADER_END;
			}
			else if (l == ':') {
				header->censure = FALSE;
				header->state   = HEADER_VALUE;
			}
			else {
				header->currHeader[header->headerIndex++] = tolower(l);
				header->state							  = HEADER_NAME;
			}
			break;
		case HEADER_NAME:
			if (l == ':') {
				header->currHeader[header->headerIndex++] = 0;
				isCensureHeader(header);
				header->state = HEADER_VALUE;
			}
			else {
				header->currHeader[header->headerIndex++] = tolower(l);
			}

			if (header->headerIndex == MAX_HOP_BY_HOP_HEADER_LENGTH) {
				header->headerIndex++; // TODO fix better
				copyBuffer(header);
				header->headerIndex = 0;
				header->state		= HEADER_VALUE;
			}
			break;
		case HEADER_VALUE:
			if (l == '\n') {
				header->state = HEADER_DONE;
			}
			else {
				if (l != '\t' && l != ' ') {
					header->isChar = TRUE;
				}
				if (header->isMime && l != '\r') {
					if (!isOWS(l) && l != ';' &&
						header->mediaRangeCurrent != -1) {
						header->transformContent =
							doesMatchAt((header->mediaRangeCurrent)++, l,
										header->mediaRange);
					}
					else if (header->mediaRangeCurrent != 0) {
						header->mediaRangeCurrent = -1;
					}
				}
				else if (header->isTransfer && l != '\r' &&
						 header->tranferIndex <= CHUNKED_LENGTH &&
						 header->isChar) {
					header->transferValue[header->tranferIndex++] = tolower(l);
					if (header->tranferIndex == CHUNKED_LENGTH) {
						header->transferValue[header->tranferIndex++] = 0;
						compareWithChunked(header);
					}
				}
				else if (header->isEncoded && l != '\r' &&
						 header->contentIndex <= IDENTITY_LENGTH &&
						 header->isChar) {
					header->contentValue[header->contentIndex++] = tolower(l);
					if (header->contentIndex == IDENTITY_LENGTH) {
						header->contentValue[header->contentIndex++] = 0;
						compareWithIdentity(header);
					}
				}
			}
			if (!header->censure) {
				buffer_write(&header->valueBuffer, l);
			}
			break;
		case HEADER_END:
			if (l == '\n') {
				header->state = BODY_START;
			}
			else if (l != ':') {
				header->currHeader[header->headerIndex++] = tolower(l);
				header->state							  = HEADER_NAME;
			}
			else {
				header->state = HEADER_VALUE;
			}

			break;
		case HEADER_DONE:
		case BODY_START:
			break;
	}

	if (header->headerIndex > MAX_HOP_BY_HOP_HEADER_LENGTH &&
		header->state != HEADER_VALUE) {
		header->censure = FALSE;
	}
}

void resetHeaderParser(struct headersParser *header) {
	header->headerIndex  = 0;
	header->tranferIndex = 0;
	header->mimeIndex	= 0;
	header->contentIndex = 0;
	header->state		 = HEADERS_START;
	header->censure		 = FALSE;
	header->isMime		 = FALSE;
	header->isTransfer   = FALSE;
	header->isChar		 = FALSE;
	header->isEncoded	= FALSE;
}

static void isCensureHeader(struct headersParser *header) {
	if (header->headerIndex >= MAX_HOP_BY_HOP_HEADER_LENGTH ||
		header->headerIndex == 0) {
		header->censure = FALSE;
	}
	else if (strcmp(header->currHeader, "keep-alive") == 0 ||
			 strcmp(header->currHeader, "connection") == 0 ||
			 strcmp(header->currHeader, "upgrade") == 0 ||
			 strcmp(header->currHeader, "expect") == 0) {
		header->censure = TRUE;
	}
	else if (getIsTransformationOn(getConfiguration()) &&
			 strcmp(header->currHeader, "trailer") == 0) {
		header->censure = TRUE;
	}
	else if (getIsTransformationOn(getConfiguration()) && !header->isRequest &&
			 strcmp(header->currHeader, "transfer-encoding") == 0) {
		header->isTransfer = TRUE;
		header->censure	= TRUE;
	}
	else if (getIsTransformationOn(getConfiguration()) && !header->isRequest &&
			 strcmp(header->currHeader, "content-length") == 0) {
		header->censure = TRUE;
	}
	else if (getIsTransformationOn(getConfiguration()) && !header->isRequest &&
			 strcmp(header->currHeader, "content-encoding") == 0) {
		header->isEncoded	 = TRUE;
		header->hasEncode	 = TRUE;
		header->willTransform = FALSE;
		header->censure		  = FALSE;
		copyBuffer(header);
		buffer_write(&header->valueBuffer, ':');
	}
	else {
		if (strcmp(header->currHeader, "content-type") == 0 &&
			getIsTransformationOn(getConfiguration())) {
			header->isMime = TRUE;
		}

		copyBuffer(header);
		buffer_write(&header->valueBuffer, ':');
		header->censure = FALSE;
	}
}

int getTransformContentParser(struct headersParser *header) {
	return header->transformContent;
}

void addLastHeaders(struct headersParser *header) {
	size_t count, size;
	if (!header->isRequest && getIsTransformationOn(getConfiguration())) {
		char *newHeader2 = "transfer-encoding: chunked\r\n";
		size			 = strlen(newHeader2);
		memcpy(buffer_write_ptr(&header->valueBuffer, &count), newHeader2,
			   size);
		buffer_write_adv(&header->valueBuffer, size);
	}

	char *newHeader = "connection: close\r\n\r\n";
	size			= strlen(newHeader);
	memcpy(buffer_write_ptr(&header->valueBuffer, &count), newHeader, size);
	buffer_write_adv(&header->valueBuffer, size);
}

void copyBuffer(struct headersParser *header) {
	size_t count;
	memcpy(buffer_write_ptr(&header->valueBuffer, &count), header->currHeader,
		   header->headerIndex - 1);
	buffer_write_adv(&header->valueBuffer, header->headerIndex - 1);
}

void resetValueBuffer(struct headersParser *header) {
	if (!buffer_can_read(&header->valueBuffer)) {
		header->valueIndex = 0;
	}
}

void compareWithChunked(struct headersParser *header) {
	if (strcmp((char *) header->transferValue, "chunked") == 0) {
		header->isChunked = TRUE;
	}
}

void compareWithIdentity(struct headersParser *header) {
	if (strcmp((char *) header->contentValue, "identity") == 0) {
		header->willTransform = TRUE;
	}
}

uint8_t getTransformEncode(struct headersParser *header) {
	if (!header->hasEncode || header->willTransform) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

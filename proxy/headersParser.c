#include <stdio.h>
#include <stdlib.h>
#include <headersParser.h>
#include <configuration.h>
#include <utilities.h>
#include <string.h>
#include <ctype.h>

static void isCensureHeader(struct headersParser *header);

void headersParserInit(struct headersParser *header) {
	header->state		= FIRST_LINE;
	header->headerIndex = 0;
	header->valueIndex  = 0;
	header->mimeIndex   = 0;
	header->censure		= FALSE;
	header->isMime		= FALSE;
	buffer_init(&(header->headerBuffer), MAX_HEADER_LENGTH, header->headerBuf);
	buffer_init(&(header->valueBuffer), 20 + 20 + MAX_HOP_BY_HOP_HEADER_LENGTH,
				header->valueBuf); // TODO update with configuration buffer size
}

void parseHeaders(struct headersParser *header, buffer *input, int begining,
				  int end) {
	uint8_t l = buffer_read(input);
	while (l) {
		parseHeadersByChar(l, header);

		if (header->state == HEADER_DONE) {
			printf("%s\n", header->currHeader);
			resetHeaderParser(header);
		}
		else if (header->state == BODY_START) {
			addConnectionClose(header);
			printf("body start\n");
			return;
		}
		if (!buffer_can_write(&header->valueBuffer)) {
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
				header->headerIndex++; // TODO fix better el
				// parche
				copyBuffer(header);
				header->headerIndex = 0;
			}
			break;
		case HEADER_VALUE:
			if (l == '\n') {
				if (header->isMime) {
					header->mimeValue[header->mimeIndex++] = 0;
					printf("mimeType: %s\n", header->mimeValue); // TODO remove
				}
				header->state = HEADER_DONE;
			}
			else {
				if (header->isMime && l != '\r') {
					header->mimeValue[header->mimeIndex++] = l;
				}
				else if (header->isMime && l == '\r') {
					header->mimeValue[header->mimeIndex++] = 0;
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
	header->headerIndex = 0;
	header->mimeIndex   = 0;
	header->state		= HEADERS_START;
	header->censure		= FALSE;
	header->isMime		= FALSE;
}

static void isCensureHeader(struct headersParser *header) {
	if (header->headerIndex >= MAX_HOP_BY_HOP_HEADER_LENGTH ||
		header->headerIndex == 0) {
		header->censure = FALSE;
	}

	else if (strcmp(header->currHeader, "keep-alive") == 0 ||
			 strcmp(header->currHeader, "connection") == 0 ||
			 strcmp(header->currHeader, "upgrade") == 0) {
		header->censure = TRUE;
	}
	else if (getIsTransformationOn(getConfiguration()) &&
			 strcmp(header->currHeader, "trailer") == 0) {
		header->censure = TRUE;
	}
	else if (getIsTransformationOn(getConfiguration()) &&
			 strcmp(header->currHeader, "transfer-encoding") == 0) {
		char *newHeader = "transfer-encoding: chunked\r\n";
		memcpy(header->headerBuf, newHeader, strlen(newHeader));
		buffer_write_adv(&header->headerBuffer, strlen(newHeader));
		header->censure = TRUE;
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

void addConnectionClose(struct headersParser *header) {
	char *newHeader = "connection: close\r\n\r\n";
	size_t count, size;
	size = strlen(newHeader);
	memcpy(buffer_write_ptr(&header->valueBuffer, &count), newHeader, size);
	buffer_write_adv(&header->valueBuffer, size);
}

void copyBuffer(struct headersParser *header) {
	size_t count;
	memcpy(buffer_write_ptr(&header->valueBuffer, &count), header->currHeader,
		   header->headerIndex - 1); // TOdo CHECK IF ZERO IS COPIED
	buffer_write_adv(&header->valueBuffer, header->headerIndex - 1);
}

void resetValueBuffer(struct headersParser *header) {
	if (!buffer_can_read(&header->valueBuffer)) {
		header->valueIndex = 0;
	}
}

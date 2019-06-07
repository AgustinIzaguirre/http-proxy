#include <stdio.h>
#include <stdlib.h>
#include <headersParser.h>
#include <configuration.h>
#include <utilities.h>
#include <string.h>
#include <ctype.h>

static void isCensureHeader(struct headersParser *header);

void headersParserInit(struct headersParser *header) {
	header->state		= HEADERS_START;
	header->headerIndex = 0;
	header->mimeIndex   = 0;
	header->censure		= FALSE;
	header->isMime		= FALSE;
	buffer_init(&(header->headerBuffer), MAX_HEADER_LENGTH, header->headerBuf);
}

void parseHeaders(struct headersParser *header, buffer *input, int begining,
				  int end) {
	while (begining < end) {
		if ((header->state != HEADER_VALUE &&
			 header->headerIndex < MAX_HOP_BY_HOP_HEADER_LENGTH) ||
			header->censure) {
			buffer_read_adv(input, 1);
		}

		parseHeadersByChar(input->data[begining], header);
		begining++;

		if (header->state == HEADER_DONE) {
			printf("%s\n", header->currHeader);
			resetHeaderParser(header);
		}
		else if (header->state == BODY_START) {
			addConnectionClose(header);
			printf("body start\n");
		}
	}
}

void parseHeadersByChar(char l, struct headersParser *header) {
	int state = header->state;
	switch (state) {
		case HEADERS_START:
			if (l == '\n') {
				header->state = BODY_START;
			}
			else if (l == '\r') {
				header->state = HEADER_END;
			}
			else if (l != ':') {
				header->currHeader[header->headerIndex++] = tolower(l);
				header->state							  = HEADER_NAME;
			}
			else {
				header->censure = FALSE;
				header->state   = HEADER_VALUE;
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
				copyBuffer(header);
			}

			break;

		case HEADER_VALUE:
			if (l == '\n') {
				if (header->isMime) {
					header->mimeValue[header->mimeIndex++] = 0;
				}
				header->state = HEADER_DONE;
			}
			else if (header->isMime) {
				header->mimeValue[header->mimeIndex++] = l;
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
		if (strcmp(header->headerBuf, "content-type") == 0) {
			printf("matcheo con content-type\n");
			header->isMime = TRUE;
		}
		printf("no matcheo %s\n", header->headerBuf);
		memcpy(header->headerBuf, header->currHeader, header->headerIndex);
		header->headerBuf[header->headerIndex] = ':';
		buffer_write_adv(&header->headerBuffer, header->headerIndex + 1);
		header->censure = FALSE;
	}
}

void addConnectionClose(struct headersParser *header) {
	char *newHeader = "connection: close\r\n\r\n";
	memcpy(header->headerBuf, newHeader, strlen(newHeader));
	buffer_write_adv(&header->headerBuffer, strlen(newHeader));
}

void copyBuffer(struct headersParser *header) {
	memcpy(header->headerBuf, header->currHeader, header->headerIndex);
	buffer_write_adv(&header->headerBuffer, header->headerIndex);
}

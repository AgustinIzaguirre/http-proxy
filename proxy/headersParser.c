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
	header->censure		= TRUE;
	buffer_init(&(header->headerBuffer), MAX_HEADER_LENGTH, header->headerBuf);
}

void initializeHeaderParser(struct headersParser **header) {
	*header = malloc(sizeof(struct headersParser));
	// validate malloc TODO
	(*header)->state	   = HEADERS_START;
	(*header)->headerIndex = 0;
	(*header)->censure	 = TRUE;
	buffer_init(&((*header)->headerBuffer), MAX_TOTAL_HEADER_LENGTH,
				(*header)->headerBuf);
}

void parseHeaders(struct headersParser *header, buffer *input, int begining,
				  int end) {
	while (begining < end) {
		parseHeadersByChar(input->data[begining], header);
		begining++;

		if (header->state == HEADER_DONE) {
			printf("%s\n", header->currHeader);
			resetHeaderParser(header);
		}
		else if (header->state == BODY_START) {
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

			break;

		case HEADER_VALUE:
			if (l == '\n') {
				header->state = HEADER_DONE;
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
	header->state		= HEADERS_START;
	header->censure		= TRUE;
}

static void isCensureHeader(struct headersParser *header) {
	if (header->headerIndex > MAX_HOP_BY_HOP_HEADER_LENGTH ||
		header->headerIndex == 0) {
		header->censure = FALSE;
	}
	else if (strcmp(header->currHeader, "connection") == 0 ||
			 strcmp(header->currHeader, "keep-alive") == 0 ||
			 // strcmp(header->currHeader, "trailer") == 0 || only for transform
			 // TODO
			 strcmp(header->currHeader, "upgrade") == 0 ||
			 strcmp(header->currHeader, "Transfer-Encoding") == 0) {
		header->censure = TRUE;
	}
	else {
		memcpy(header->headerBuf, header->currHeader, header->headerIndex);
		buffer_write_adv(&header->headerBuffer, header->headerIndex);
		// buffer_read_adv(readBuffer, header->headerIndex);
		header->censure = FALSE;
	}
}
#include <stdio.h>
#include <stdlib.h>
#include <headersParser.h>

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

void initializeHeaderParser(struct headersParser **header) {
	*header = malloc(sizeof(struct headersParser));
	// validate malloc
	(*header)->state	   = HEADERS_START;
	(*header)->headerIndex = 0;
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
				header->currHeader[header->headerIndex++] = l;
				header->state							  = HEADER_NAME;
			}
			else {
				header->state = HEADER_VALUE;
			}

			break;

		case HEADER_NAME:
			if (l == ':') {
				header->currHeader[header->headerIndex++] = 0;
				header->state							  = HEADER_VALUE;
			}
			else {
				header->currHeader[header->headerIndex++] = l;
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
				header->currHeader[header->headerIndex++] = l;
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
}

void resetHeaderParser(struct headersParser *header) {
	header->headerIndex = 0;
	header->state		= HEADERS_START;
}
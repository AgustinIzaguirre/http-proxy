#ifndef HEADER_PARSER_H
#define HEADER_PARSER_H

#include <stdint.h>
#include <buffer.h>
#include <utilities.h>

enum headerState {
    START_H,
    H,
    HO,
    HOS,
    HOST,
    HOST_DDOTS,
    IPSIX,
    END_IPSIX,
    IPFOUR_OR_HOST_NAME,
    PORT,
    FINISH,
    NOT_HEADER_HOST,
    CR,
    OWS_H,
    ERROR
};

struct headerParser {
	enum headerState state;
	int charactersRead;
	char *host;
	unsigned int sizeHost;
	int port;
    int hasFoundHost;
};

#define BLOCK 10
#define PORT_DEFAULT 80

/*
 * Initialize parser
 */
void parseHeaderInit(struct headerParser *parser);

/*
 * Parse the given input until there is nothing to read on
 * the input buffer or finds a host
 */
int parseHeader(struct headerParser *parser, buffer *input);

/*
 * Returns the current state of the header parser
 */
unsigned getHeaderState(struct headerParser *parser);

/*
 * Returns the host found by the state machine
 * Does not work if the method hasFoundHost return false
 */
char *getHostHeaderParser(struct headerParser *parser);

/*
 * Returns the port found by the state machine
 */
int getPortHeaderParser(struct headerParser *parser);

/*
 * Returns true if the parser has found a host and false otherwise
 */
int hasFoundHostHeaderParser(struct headerParser *parser);

#endif

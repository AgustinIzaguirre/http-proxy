#ifndef HOST_HEADER_PARSER_H
#define HOST_HEADER_PARSER_H

#include <stdint.h>
#include <buffer.h>
#include <utilities.h>

enum headerState {
	START_H,
	H_H,
	HO,
	HOS,
	HOST,
	HOST_DDOTS,
	IPSIX,
	END_IPSIX,
	IPFOUR_OR_HOST_NAME,
	START_PORT,
	PORT,
	FINISH,
	NOT_HEADER_HOST,
	CR_H,
	OWS_H,
	ERROR_H
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
 * Parse a char
 * Returns true if the parse has finish and false otherwise
 */
int parseHeaderChar(struct headerParser *parser, char l);

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

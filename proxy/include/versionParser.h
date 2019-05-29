#ifndef VERSION_PARSER_H
#define VERSION_PARSER_H

#include <stdint.h>
#include <buffer.h>
#include <stdlib.h>

enum versionState { // Put _V so it does not collide with others
	START_V,
	H_V,
	HT,
	HTT,
	HTTP,
	HTTP_BAR,
	VERSION_ONE,
	DOT,
	VERSION_TWO,
	FINISH_V,
	ERROR_V
};

struct versionParser {
	enum versionState state;
	int charactersRead;
	int *version;
};

/*
 * Initialize parser
 */
void parseVersionInit(struct versionParser *parser);

/*
 * Parse a char
 * Returns true if the parse has finish and false otherwise
 */
int parseVersionChar(struct versionParser *parser, char l);

/*
 * Returns the current state of the version parser
 */
unsigned getVersionState(struct versionParser *parser);

/*
 * Returns the version found by the state machine
 * The return value is a array of int with size 2
 * If the version is 1.0 the in the first position
 * of the array is the number 1 and in the second
 * the number 0
 */
int *getVersionVersionParser(struct versionParser *parser);

#endif

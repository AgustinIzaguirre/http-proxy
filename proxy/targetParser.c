#include <targetParser.h>
#include <stdio.h> //TODO:remove

#define isDigit(a) ('0' <= a && a <= '9')

int parseTargetChar(struct targetParser *parser, char l);
enum targetState startTTransition(struct targetParser *parser, char l);
enum targetState aAuthOrASchemaTransition(struct targetParser *parser, char l);
enum targetState BarASchemaTransition(struct targetParser *parser, char l);
enum targetState DBarASchemaTransition(struct targetParser *parser, char l);
enum targetState AUserinfoTransition(struct targetParser *parser, char l);
enum targetState PortTransition(struct targetParser *parser, char l);

void parseTargetInit(struct targetParser *parser) {
	parser->state		   = START_T;
	parser->charactersRead = 0;
	parser->host		   = NULL;
	parser->sizeHost	   = 0;
	parser->target		   = NULL;
	parser->sizeTarget	 = 0;
	parser->port		   = 0;
}

int parseTarget(struct targetParser *parser, buffer *input) {
	uint8_t letter;
	int ret;

	do {
		letter = buffer_read(input);

		if (letter) {
			ret = parseTargetChar(parser, letter);
		}
		printf("%c\n", letter); // TODO:remove
	} while (((ret == 0) && letter));

	return ret;
}

int parseTargetChar(struct targetParser *parser, char l) {
	parser->charactersRead++;
	int total = 0;

	if (l == ' ') {
		parser->state = END_T;
		parser->target =
			addCharToString(parser->target, &parser->sizeTarget, '\0');
		parser->host = addCharToString(parser->host, &parser->sizeHost, '\0');
	}
	else {
		parser->target =
			addCharToString(parser->target, &parser->sizeTarget, l);
	}
	switch (parser->state) {
		case START_T:
			parser->state = startTTransition(parser, l);
			break;
		case A_AUTH_OR_A_SCHEMA:
			parser->state = aAuthOrASchemaTransition(parser, l);
			break;
		case BAR_A_SCHEMA:
			parser->state = BarASchemaTransition(parser, l);
			break;
		case D_BAR_A_SCHEMA:
			parser->state = DBarASchemaTransition(parser, l);
			break;
		case A_USERINFO:
			parser->state = AUserinfoTransition(parser, l);
			break;
		case PORT_T:
			parser->state = PortTransition(parser, l);
			break;
		case A_AUTH:
			parser->state = A_AUTH;
			break;
		case END_T:
			total = parser->charactersRead;
			break;
	}

	return total;
}

unsigned getTargetState(struct targetParser *parser) {
	return parser->state;
}

char *getHost(struct targetParser *parser) {
	return parser->host;
}

char *getTarget(struct targetParser *parser) {
	return parser->target;
}

int getPortTarget(struct targetParser *parser) {
	return parser->port;
}

enum targetState startTTransition(struct targetParser *parser, char l) {
	enum targetState state;
	if (l == '/') {
		state = A_AUTH;
		free(parser->host);
		parser->host	 = NULL;
		parser->sizeHost = 0;
	}
	else if (l == ':') {
		state = A_AUTH_OR_A_SCHEMA;
	}
	else {
		state		 = START_T;
		parser->host = addCharToString(parser->host, &parser->sizeHost, l);
	}
	return state;
}

enum targetState aAuthOrASchemaTransition(struct targetParser *parser, char l) {
	enum targetState state;
	if (isDigit(l)) {
		state		 = PORT_T;
		parser->port = l - '0';
	}
	else if (l == '/') {
		state = BAR_A_SCHEMA;
		free(parser->host);
		parser->host	 = NULL;
		parser->sizeHost = 0;
	}
	else {
		state = A_AUTH;
		free(parser->host);
		parser->host	 = NULL;
		parser->sizeHost = 0;
	}
	return state;
}

enum targetState BarASchemaTransition(struct targetParser *parser, char l) {
	enum targetState state;
	if (l == '/') {
		state = D_BAR_A_SCHEMA;
	}
	else {
		state = A_AUTH;
	}
	return state;
}

enum targetState DBarASchemaTransition(struct targetParser *parser, char l) {
	enum targetState state;
	if (l == ':') {
		state = PORT_T;
	}
	else if (l == '@') {
		state = A_USERINFO;
		free(parser->host);
		parser->host	 = NULL;
		parser->sizeHost = 0;
	}
	else if (l == '/') {
		state = A_AUTH;
	}
	else {
		state		 = D_BAR_A_SCHEMA;
		parser->host = addCharToString(parser->host, &parser->sizeHost, l);
	}
	return state;
}

enum targetState AUserinfoTransition(struct targetParser *parser, char l) {
	enum targetState state;
	if (l == ':') {
		state = PORT_T;
	}
	else if (l == '/') {
		state = A_AUTH;
	}
	else {
		state		 = A_USERINFO;
		parser->host = addCharToString(parser->host, &parser->sizeHost, l);
	}
	return state;
}

enum targetState PortTransition(struct targetParser *parser, char l) {
	enum targetState state;
	if (isDigit(l)) {
		state		 = PORT_T;
		parser->port = parser->port * 10 + l - '0';
	}
	else {
		state = A_AUTH;
		if (l != '/') {
			parser->port = PORT_DEFAULT;
		}
	}
	return state;
}

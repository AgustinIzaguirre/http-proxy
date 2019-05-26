#include <headerParser.h>
#include <stdio.h>

#define isDigit(a) ('0' <= a && a <= '9')
#define isOWS(a) (a == '\t' || a == ' ')

enum headerState startTransition(struct headerParser *parser, char l);
enum headerState hTransition(struct headerParser *parser, char l);
enum headerState hoTransition(struct headerParser *parser, char l);
enum headerState hosTransition(struct headerParser *parser, char l);
enum headerState hostTransition(struct headerParser *parser, char l);
enum headerState hostDDotsTransition(struct headerParser *parser, char l);
enum headerState ipSixTransition(struct headerParser *parser, char l);
enum headerState endIpSixTransition(struct headerParser *parser, char l);
enum headerState ipFourOrHostNameTransition(struct headerParser *parser,
											char l);
enum headerState portTransition(struct headerParser *parser, char l);
enum headerState crTransition(struct headerParser *parser, char l);
enum headerState notHeaderHostTransition(struct headerParser *parser, char l);
enum headerState owsHTransition(struct headerParser *parser, char l);

void parseHeaderInit(struct headerParser *parser) {
	parser->state		   = START_H;
	parser->charactersRead = 0;
	parser->host		   = NULL;
	parser->sizeHost	   = 0;
	parser->port		   = PORT_DEFAULT;
	parser->hasFoundHost   = FALSE;
}

unsigned getHeaderState(struct headerParser *parser) {
	return parser->state;
}

char *getHostHeaderParser(struct headerParser *parser) {
	return parser->host;
}

int getPortHeaderParser(struct headerParser *parser) {
	return parser->port;
}

int hasFoundHostHeaderParser(struct headerParser *parser) {
	return parser->hasFoundHost;
}

int parseHeaderChar(struct headerParser *parser, char l) {
	parser->charactersRead++;
	int flag = 1;

	switch (parser->state) {
		case START_H:
			parser->state = startTransition(parser, l);
			break;
		case H_H:
			parser->state = hTransition(parser, l);
			break;
		case HO:
			parser->state = hoTransition(parser, l);
			break;
		case HOS:
			parser->state = hosTransition(parser, l);
			break;
		case HOST:
			parser->state = hostTransition(parser, l);
			break;
		case HOST_DDOTS:
			parser->state = hostDDotsTransition(parser, l);
			break;
		case IPSIX:
			parser->state = ipSixTransition(parser, l);
			break;
		case END_IPSIX:
			parser->state = endIpSixTransition(parser, l);
			break;
		case IPFOUR_OR_HOST_NAME:
			parser->state = ipFourOrHostNameTransition(parser, l);
			break;
		case PORT:
			parser->state = portTransition(parser, l);
			break;
		case NOT_HEADER_HOST:
			parser->state = notHeaderHostTransition(parser, l);
			break;
		case CR_H:
			parser->state = crTransition(parser, l);
			break;
		case OWS_H:
			parser->state = owsHTransition(parser, l);
			break;
		case FINISH:
			if (l == '\n') {
				parser->hasFoundHost = TRUE;
			}
			else {
				parser->state = ERROR_H;
			}
		case ERROR_H:
			flag = 0;
			break;
	}

	return flag;
}

enum headerState startTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == 'H' || l == 'h') {
		state = H_H;
	}
	return state;
}

enum headerState hTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == 'O' || l == 'o') {
		state = HO;
	}
	return state;
}

enum headerState hoTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == 'S' || l == 's') {
		state = HOS;
	}
	return state;
}

enum headerState hosTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == 'T' || l == 't') {
		state = HOST;
	}
	return state;
}

enum headerState hostTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == ':') {
		state = HOST_DDOTS;
	}
	return state;
}

enum headerState hostDDotsTransition(struct headerParser *parser, char l) {
	enum headerState state = HOST_DDOTS;
	if (l == '[') {
		state = IPSIX;
	}
	else if (!isOWS(l)) {
		state		 = IPFOUR_OR_HOST_NAME;
		parser->host = addCharToString(parser->host, &parser->sizeHost, l);
	}
	return state;
}

enum headerState ipSixTransition(struct headerParser *parser, char l) {
	enum headerState state = IPSIX;
	if (l == ']') {
		state = END_IPSIX;
		l	 = '\0';
	}
	parser->host = addCharToString(parser->host, &parser->sizeHost, l);
	return state;
}

enum headerState endIpSixTransition(struct headerParser *parser, char l) {
	enum headerState state = ERROR_H;
	if (l == '\r') {
		state = FINISH;
	}
	else if (l == ':') {
		state		 = PORT;
		parser->port = 0;
	}
	else if (isOWS(l)) {
		state = OWS_H;
	}
	return state;
}

enum headerState ipFourOrHostNameTransition(struct headerParser *parser,
											char l) {
	enum headerState state = IPFOUR_OR_HOST_NAME;
	if (l == '\r') {
		state = FINISH;
		l	 = '\0';
	}
	else if (l == ':') {
		state		 = PORT;
		l			 = '\0';
		parser->port = 0;
	}
	else if (isOWS(l)) {
		state = OWS_H;
		l	 = '\0';
	}
	parser->host = addCharToString(parser->host, &parser->sizeHost, l);
	return state;
}

enum headerState portTransition(struct headerParser *parser, char l) {
	enum headerState state = PORT;
	if (l == '\r') {
		state = FINISH;
	}
	else if (!isDigit(l)) {
		state = ERROR_H;
	}
	else {
		parser->port = parser->port * 10 + l - '0';
	}
	return state;
}

enum headerState crTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == '\n') {
		state = START_H;
	}
	return state;
}

enum headerState notHeaderHostTransition(struct headerParser *parser, char l) {
	enum headerState state = NOT_HEADER_HOST;
	if (l == '\r') {
		state = CR_H;
	}
	return state;
}

enum headerState owsHTransition(struct headerParser *parser, char l) {
	enum headerState state = OWS_H;
	if (l == '\r') {
		state = FINISH;
	}
	else if (!isOWS(l)) {
		state = ERROR_H;
	}
	return state;
}

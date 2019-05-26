#include <methodParser.h>

static void transitionStart(struct methodParser *parser, char l);

void parseMethodInit(struct methodParser *parser) {
	parser->state		   = START;
	parser->charactersRead = 0;
	parser->method		   = NO_METHOD;
}

int parseMethodChar(struct methodParser *parser, char l) {
	unsigned state = parser->state;
	parser->charactersRead++;
	int flag = 1;

	switch (state) {
		case START:
			transitionStart(parser, l);
			break;

		case CR:
			if (l == '\n') {
				parser->state = START;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case G:
			if (l == 'E') {
				parser->state = GE;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case GE:
			if (l == 'T') {
				parser->state = GET;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case GET:
			if (l == ' ') {
				parser->method = GET_METHOD;
				parser->state  = DONE_METHOD_STATE;
				flag		   = 0;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case H:
			if (l == 'E') {
				parser->state = HE;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case HE:
			if (l == 'A') {
				parser->state = HEA;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case HEA:
			if (l == 'D') {
				parser->state = HEAD;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case HEAD:
			if (l == ' ') {
				parser->method = HEAD_METHOD;
				parser->state  = DONE_METHOD_STATE;
				flag		   = 0;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case P:
			if (l == 'O') {
				parser->state = PO;
			}
			else if (l == 'U') {
				parser->state = PU;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}
			break;

		case PO:
			if (l == 'S') {
				parser->state = POS;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case POS:
			if (l == 'T') {
				parser->state = POST;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case POST:
			if (l == ' ') {
				parser->method = POST_METHOD;
				parser->state  = DONE_METHOD_STATE;
				flag		   = 0;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case PU:
			if (l == 'T') {
				parser->state = PUT;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case PUT:
			if (l == ' ') {
				parser->method = PUT_METHOD;
				parser->state  = DONE_METHOD_STATE;
				flag		   = 0;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case D:
			if (l == 'E') {
				parser->state = DE;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case DE:
			if (l == 'L') {
				parser->state = DEL;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case DEL:
			if (l == 'E') {
				parser->state = DELE;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case DELE:
			if (l == 'T') {
				parser->state = DELET;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case DELET:
			if (l == 'E') {
				parser->state = DELETE;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case DELETE:
			if (l == ' ') {
				parser->method = DELETE_METHOD;
				parser->state  = DONE_METHOD_STATE;
				flag		   = 0;
			}
			else {
				parser->state = ERROR_METHOD_STATE;
			}

			break;

		case DONE_METHOD_STATE:
		case ERROR_METHOD_STATE:
			flag = 0;
			break;
	}

	return flag;
}

static void transitionStart(struct methodParser *parser, char l) {
	if (l == 'G') {
		parser->method = G;
		parser->state  = G;
	}
	else if (l == 'H') {
		parser->method = H;
		parser->state  = H;
	}
	else if (l == 'D') {
		parser->method = D;
		parser->state  = D;
	}
	else if (l == 'P') {
		parser->method = P;
		parser->state  = P;
	}
	else if (l == '\r') {
		parser->method = CR;
	}
	else if (l != '\n') {
		parser->method = ERROR_METHOD_STATE;
	}
}

unsigned getState(struct methodParser *parser) {
	return parser->state;
}

unsigned getMethod(struct methodParser *parser) {
	return parser->method;
}

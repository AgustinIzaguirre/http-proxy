#include <versionParser.h>
#include <stdio.h>

#define isDigit(a) ('0' <= a && a <= '9')

static enum versionState startVTransition(struct versionParser *parser, char l);
static enum versionState hVTransition(struct versionParser *parser, char l);
static enum versionState htTransition(struct versionParser *parser, char l);
static enum versionState httTransition(struct versionParser *parser, char l);
static enum versionState httpTransition(struct versionParser *parser, char l);
static enum versionState httpBarTransition(struct versionParser *parser,
										   char l);
static enum versionState versionOneransition(struct versionParser *parser,
											 char l);
static enum versionState dotTransition(struct versionParser *parser, char l);
static enum versionState versionTwoTransition(struct versionParser *parser,
											  char l);

void parseVersionInit(struct versionParser *parser) {
	parser->state		   = START_V;
	parser->charactersRead = 0;
	parser->version		   = calloc(2, sizeof(int));
}

unsigned getVersionState(struct versionParser *parser) {
	return parser->state;
}

int *getVersionVersionParser(struct versionParser *parser) {
	return parser->version;
}

int parseVersionChar(struct versionParser *parser, char l) {
	parser->charactersRead++;
	int flag = 1;

	do {
		switch (parser->state) {
			case START_V:
				parser->state = startVTransition(parser, l);
				break;
			case H_V:
				parser->state = hVTransition(parser, l);
				break;
			case HT:
				parser->state = htTransition(parser, l);
				break;
			case HTT:
				parser->state = httTransition(parser, l);
				break;
			case HTTP:
				parser->state = httpTransition(parser, l);
				break;
			case HTTP_BAR:
				parser->state = httpBarTransition(parser, l);
				break;
			case VERSION_ONE:
				parser->state = versionOneransition(parser, l);
				break;
			case DOT:
				parser->state = dotTransition(parser, l);
				break;
			case VERSION_TWO:
				parser->state = versionTwoTransition(parser, l);
				break;
			case FINISH_V:
				if (l == '\n') {
					flag = 0;
					break;
				}
				parser->state = ERROR_V;
			case ERROR_V:
				flag = 0;
				break;
		}
	} while (parser->state == FINISH_V && flag);

	return flag;
}

enum versionState startVTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (l == 'H') {
		state = H_V;
	}
	return state;
}

enum versionState hVTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (l == 'T') {
		state = HT;
	}
	return state;
}

enum versionState htTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (l == 'T') {
		state = HTT;
	}
	return state;
}

enum versionState httTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (l == 'P') {
		state = HTTP;
	}
	return state;
}

enum versionState httpTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (l == '/') {
		state = HTTP_BAR;
	}
	return state;
}

enum versionState httpBarTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (isDigit(l)) {
		state				 = VERSION_ONE;
		(parser->version)[0] = l - '0';
	}
	return state;
}

enum versionState dotTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (isDigit(l)) {
		state				 = VERSION_TWO;
		(parser->version)[1] = l - '0';
	}
	return state;
}

enum versionState versionOneransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (isDigit(l)) {
		state				 = VERSION_ONE;
		(parser->version)[0] = 10 * (parser->version)[0] + l - '0';
	}
	else if (l == '.') {
		state = DOT;
	}
	return state;
}

enum versionState versionTwoTransition(struct versionParser *parser, char l) {
	enum versionState state = ERROR_V;
	if (isDigit(l)) {
		state				 = VERSION_TWO;
		(parser->version)[1] = 10 * (parser->version[1]) + l - '0';
	}
	else if (l == '\r' || l == '\n') {
		state = FINISH_V;
	}

	return state;
}

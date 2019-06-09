#include <handleParsers.h>
#include <connectToOrigin.h>

static int parse(struct parseRequest *parseRequest, buffer *input, int *flag,
				 int (*parseChar)(struct parseRequest *, char));
static int handleMethod(struct selector_key *key,
						struct parseRequest *parseRequest, buffer *readBuffer,
						unsigned *ret);
static int handleTarget(struct selector_key *key,
						struct parseRequest *parseRequest, buffer *readBuffer,
						unsigned *ret);
static int handleVersion(struct selector_key *key,
						 struct parseRequest *parseRequest, buffer *readBuffer,
						 unsigned *ret);
static int handleHeader(struct selector_key *key,
						struct parseRequest *parseRequest, buffer *readBuffer,
						unsigned *ret);
static int parseMethodCharWrapper(struct parseRequest *parseRequest,
								  char letter);
static int parseTargetCharWrapper(struct parseRequest *parseRequest,
								  char letter);
static int parseVersionCharWrapper(struct parseRequest *parseRequest,
								   char letter);
static int parseHeaderCharWrapper(struct parseRequest *parseRequest,
								  char letter);
static unsigned parseProcess(struct selector_key *key, buffer *readBuffer,
							 size_t bytesRead);

void parseInit(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	parseRequest->input				 = getReadBuffer(GET_DATA(key));
	parseRequest->finishParserBuffer = getFinishParserBuffer(GET_DATA(key));

	parseMethodInit(&(parseRequest->methodParser));
	parseTargetInit(&(parseRequest->targetParser));
	parseVersionInit(&(parseRequest->versionParser));
	parseHeaderInit(&(parseRequest->headerParser));
}

void parseDestroy(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));
	setRequestMethod(GET_DATA(key), getMethod(&(parseRequest->methodParser)));
	// TODO:maybe more things
}

unsigned parseRead(struct selector_key *key) {
	buffer *readBuffer = getReadBuffer(GET_DATA(key));
	unsigned ret	   = PARSE;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (buffer_can_write(readBuffer)) {
		pointer   = buffer_write_ptr(readBuffer, &count);
		bytesRead = recv(key->fd, pointer, count, 0);

		if (bytesRead >= 0) {
			buffer_write_adv(readBuffer, bytesRead);
			ret = parseProcess(key, readBuffer, bytesRead);
		}
		else {
			ret = ERROR;
		}
	}

	return ret;
}

unsigned parseProcess(struct selector_key *key, buffer *readBuffer,
					  size_t bytesRead) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));
	unsigned ret					  = PARSE;
	int bytesProcessed				  = 0;
	int bytesFromAParser			  = 0;
	while (bytesRead != bytesProcessed && ret == PARSE) {
		switch (parseRequest->state) {
			case PARSE_METHOD:
				bytesFromAParser =
					handleMethod(key, parseRequest, readBuffer, &ret);
				break;
			case PARSE_TARGET:
				bytesFromAParser =
					handleTarget(key, parseRequest, readBuffer, &ret);
				break;
			case PARSE_VERSION:
				bytesFromAParser =
					handleVersion(key, parseRequest, readBuffer, &ret);
				break;
			case PARSE_HEADER:
				bytesFromAParser =
					handleHeader(key, parseRequest, readBuffer, &ret);
				break;
		}
		bytesProcessed = bytesProcessed + bytesFromAParser;
	}
	return ret;
}

int handleMethod(struct selector_key *key, struct parseRequest *parseRequest,
				 buffer *readBuffer, unsigned *ret) {
	int state = 0;
	int bytesFromAParser =
		parse(parseRequest, readBuffer, &state, &parseMethodCharWrapper);
	if (state == 1) {
		parseRequest->state = PARSE_TARGET;
		if (getState(&(parseRequest->methodParser)) == ERROR_METHOD_STATE) {
			setErrorType(GET_DATA(key), METHOD_NOT_ALLOW);
			*ret = ERROR_CLIENT;
		}
	}
	else if (state == 2) {
		setErrorType(GET_DATA(key), NOT_FOUND_HOST);
		*ret = ERROR_CLIENT;
	}
	return bytesFromAParser;
}

int handleTarget(struct selector_key *key, struct parseRequest *parseRequest,
				 buffer *readBuffer, unsigned *ret) {
	int state = 0;
	int bytesFromAParser =
		parse(parseRequest, readBuffer, &state, &parseTargetCharWrapper);
	if (state == 1) {
		parseRequest->state = PARSE_VERSION;
		char *host			= getHost(&(parseRequest->targetParser));
		if (host != NULL && host[0] != 0) {
			setOriginHost(GET_DATA(key), host); // set host
			int port = getPortTarget(&(parseRequest->targetParser));
			setOriginPort(GET_DATA(key), port);
		}
	}
	else if (state == 2) {
		setErrorType(GET_DATA(key), NOT_FOUND_HOST);
		*ret = ERROR_CLIENT;
	}
	return bytesFromAParser;
}

int handleVersion(struct selector_key *key, struct parseRequest *parseRequest,
				  buffer *readBuffer, unsigned *ret) {
	int state = 0;
	int bytesFromAParser =
		parse(parseRequest, readBuffer, &state, &parseVersionCharWrapper);
	if (state == 1) {
		if (getVersionState(&parseRequest->versionParser) == ERROR_V) {
			*ret = ERROR_CLIENT;
		}
		else if (getOriginHost(GET_DATA(key)) == NULL ||
				 getOriginHost(GET_DATA(key))[0] == 0) {
			parseRequest->state = PARSE_HEADER;
			int *version =
				getVersionVersionParser(&parseRequest->versionParser);
			if (version[0] != 1 || (version[1] != 0 && version[1] != 1)) {
				setErrorType(GET_DATA(key), VERSION_NOT_SUPPORTED);
				*ret = ERROR_CLIENT;
			}
		}
		else {
			*ret = blockingToResolvName(key, key->fd); // evans TODO
		}
	}
	else if (state == 2) {
		setErrorType(GET_DATA(key), NOT_FOUND_HOST);
		*ret = ERROR_CLIENT;
	}
	return bytesFromAParser;
}

int handleHeader(struct selector_key *key, struct parseRequest *parseRequest,
				 buffer *readBuffer, unsigned *ret) {
	int state = 0;
	int bytesFromAParser =
		parse(parseRequest, readBuffer, &state, &parseHeaderCharWrapper);
	if (state == 1) {
		if (hasFoundHostHeaderParser(&parseRequest->headerParser)) {
			char *host = getHostHeaderParser(&(parseRequest->headerParser));
			setOriginHost(GET_DATA(key), host);
			int port = getPortHeaderParser(&parseRequest->headerParser);
			if (port != -1) {
				setOriginPort(GET_DATA(key), port);
			}
			*ret = blockingToResolvName(key, key->fd); // evans TODO
		}
		else {
			setErrorType(GET_DATA(key), NOT_FOUND_HOST);
			*ret = ERROR_CLIENT;
		}
	}
	else if (state == 2) {
		setErrorType(GET_DATA(key), NOT_FOUND_HOST);
		*ret = ERROR_CLIENT;
	}
	return bytesFromAParser;
}

int parse(struct parseRequest *parseRequest, buffer *input, int *flag,
		  int (*parseChar)(struct parseRequest *, char)) {
	uint8_t letter;
	int ret = 0;
	*flag   = 1;

	do {
		letter = buffer_read(input);

		if (letter) {
			if (buffer_can_write(parseRequest->finishParserBuffer)) {
				if (parseChar(parseRequest, letter)) {
					*flag = 0;
				}
				else {
					*flag = 1;
				}
				buffer_write(parseRequest->finishParserBuffer, letter);
				ret++;
			}
			else {
				*flag = 2;
			}
		}
	} while (!(*flag) && letter);

	return ret;
}

int parseMethodCharWrapper(struct parseRequest *parseRequest, char letter) {
	return parseMethodChar(&parseRequest->methodParser, letter);
}

int parseTargetCharWrapper(struct parseRequest *parseRequest, char letter) {
	return parseTargetChar(&parseRequest->targetParser, letter);
}

int parseVersionCharWrapper(struct parseRequest *parseRequest, char letter) {
	return parseVersionChar(&parseRequest->versionParser, letter);
}

int parseHeaderCharWrapper(struct parseRequest *parseRequest, char letter) {
	return parseHeaderChar(&parseRequest->headerParser, letter);
}

#include <handleParsers.h>
#include <connectToOrigin.h>

void parseRequestInit(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	parseRequest->input = getReadBuffer(GET_DATA(key));

	parseInit(&(parseRequest->methodParser));
}

void parseRequestDestroy(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));
	setRequestMethod(GET_DATA(key), getMethod(&(parseRequest->methodParser)));
}

unsigned parseRead(struct selector_key *key) {
	buffer *readBuffer = getReadBuffer(GET_DATA(key));
	unsigned ret	   = stm_state(getStateMachine(GET_DATA(key)));
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (buffer_can_write(readBuffer)) {
		pointer   = buffer_write_ptr(readBuffer, &count);
		bytesRead = recv(key->fd, pointer, count, 0);

		if (bytesRead > 0) {
			buffer_write_adv(readBuffer, bytesRead);
			struct parseRequest *parseRequest =
				getParseRequestState(GET_DATA(key));
			switch (ret) {
				case PARSE_METHOD:
					if (parseMethod(&parseRequest->methodParser, readBuffer)) {
						ret = PARSE_TARGET;
						evans
						// ret = CONNECT_TO_ORIGIN;
						// blockingToResolvName(key, key->fd);
					}
					break;
				case PARSE_TARGET:
					if (parseTarget(&parseRequest->targetParser, readBuffer)) {
						ret = PARSE_VERSION;
					}
					break;
				case PARSE_VERSION:
					if (parseVersion(&parseRequest->versionParser,
									 readBuffer)) {
						if (getVersionState(&parseRequest->versionParser) ==
							ERROR_V) {
							ret = ERROR; // TODO: check
						}
						else if (getOriginHost(GET_DATA(key)) == NULL) {
							ret = PARSE_HEADER;
						}
						else {
							ret = DONE; // TODO: put connect and resolve state
						}
					}
				case PARSE_HEADER:
					if (parseHeader(&parseRequest->headerParser, readBuffer)) {
						if (hasFoundHostHeaderParser(
								&parseRequest
									 ->headerParser)) { // TODO: drop error
														// if buffer is full
							ret = DONE; // TODO: put connect and resolve state
						}
						else {
							ret = ERROR;
						}
					}
			}
		}
		else {
			printf("error\n"); // TODO:remove
			ret = ERROR;
		}
	}
	return ret;
}

void parseTargetArrive(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	parseRequest->input = getReadBuffer(GET_DATA(key));
	parseTargetInit(&(parseRequest->targetParser));
	printf("entrando target\n"); // TODO:remove
}

void parseTargetDeparture(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	printf("SALIENDO target\n"); // TODO:remove
	char *host = getHost(&(parseRequest->targetParser));
	if (host != NULL) {
		printf("TARGET Host:%s\n", host); // TODO: remove
		setOriginHost(GET_DATA(key), host);
		int port = getPortTarget(&(parseRequest->targetParser));
		printf("TARGET Port:%d\n", port); // TODO: remove
	}
}

void parseVersionArrive(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));
	printf("ENTRANDO version\n"); // TODO:remove

	parseRequest->input = getReadBuffer(GET_DATA(key));
	parseVersionInit(&(parseRequest->versionParser));
}

void parseVersionDeparture(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	printf("SALIENDO version\n"); // TODO:remove
	int *version = getVersionVersionParser(&(parseRequest->versionParser));
	printf("VERSION:%d.%d\n", version[0], version[1]); // TODO: remove
}

void parseHeaderArrive(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	parseRequest->input = getReadBuffer(GET_DATA(key));
	parseHeaderInit(&(parseRequest->headerParser));
}

void parseHeaderDeparture(const unsigned state, struct selector_key *key) {
	struct parseRequest *parseRequest = getParseRequestState(GET_DATA(key));

	if (hasFoundHostHeaderParser(&parseRequest->headerParser)) {
		char *host = getHostHeaderParser(&(parseRequest->headerParser));
		printf("HEADER Host:%s\n", host); // TODO: remove
		setOriginHost(GET_DATA(key), host);
		int port = getPortHeaderParser(&(parseRequest->headerParser));
		printf("HEADER Port:%d\n", port); // TODO: remove
	}
}

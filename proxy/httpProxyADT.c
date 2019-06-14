#include <http.h>
#include <httpProxyADT.h>
#include <connectToOrigin.h>
#include <handleRequest.h>
#include <handleResponse.h>
#include <handleResponseWithTransform.h>
#include <headersParser.h>
#include <transformBody.h>

static const struct state_definition *httpDescribeStates(void);

struct http {
	// Client info
	struct sockaddr_storage clientAddr;
	socklen_t clientAddrLen;
	int clientFd;

	// Origin server address resolution
	struct addrinfo *originResolution;
	// Current try origin server address
	struct addrinfo *originResolutionCurrent; // TODO: it is not use

	// Origin server info
	struct sockaddr_storage originAddr;
	socklen_t originAddrLen;
	int originDomain;
	int originFd;
	unsigned short originPort;
	char *host; // TODO: check

	// HTTP proxy state machine
	struct state_machine stm;

	// Request method
	unsigned requestMethod;

	// States Structures
	union {
		struct parseRequest parseRequest;
		struct handleRequest handleRequest;
		struct handleResponse handleResponse;
		struct handleResponseWithTransform
			handleResponseWithTransform; // TODO remove deprecated
		struct transformBody transformBody;
		int other; // TODO REMOVE
	} clientState;

	// buffers to use: readBuffer and writeBuffer.
	uint8_t raWRequest[MAX_FIRST_LINE_LENGTH],
		rawResponse[MAX_FIRST_LINE_LENGTH];
	uint8_t rawBuffA[BUFFER_SIZE],
		rawBuffB[BUFFER_SIZE]; // TODO Buffer size should be read from config
	buffer readBuffer, writeBuffer, requestLine, responseLine;

	uint8_t finishParserData[MAX_PARSER];
	buffer finishParserBuffer;

	buffer errorBuffer;
	enum errorType errorTypeFound;

	// Number of references to this object, if one destroy
	unsigned references;

	int transformContent;
	MediaRangePtr_t mediaRanges; // TODO:free

	// Next in pool
	struct http *next;
};

struct addrinfo *getOriginResolutions(struct http *s) {
	return s->originResolution;
}

void setOriginResolutions(struct http *s, struct addrinfo *originResolution) {
	s->originResolution = originResolution;
}

struct state_machine *getStateMachine(struct http *s) {
	return &(s->stm);
}

int getClientFd(struct http *s) {
	return s->clientFd;
}

int getOriginFd(struct http *s) {
	return s->originFd;
}

void setOriginFd(struct http *s, int originFd) {
	s->originFd = originFd;
}

unsigned short getOriginPort(struct http *s) {
	return s->originPort;
}

void setOriginPort(struct http *s, unsigned short originPort) {
	s->originPort = originPort;
}

struct parseRequest *getParseRequestState(httpADT_t s) {
	return &((s->clientState).parseRequest);
}

struct handleRequest *getHandleRequestState(httpADT_t s) {
	return &((s->clientState).handleRequest);
}

struct handleResponse *getHandleResponseState(httpADT_t s) {
	return &((s->clientState).handleResponse);
}

struct handleResponseWithTransform *
getHandleResponseWithTransformState(httpADT_t s) {
	return &((s->clientState).handleResponseWithTransform);
}

struct transformBody *getTransformBodyState(httpADT_t s) {
	return &((s->clientState).transformBody);
}

struct sockaddr_storage *getClientAddress(httpADT_t s) {
	return &(s->clientAddr);
}

socklen_t getClientAddressLength(httpADT_t s) {
	return s->clientAddrLen;
}

void setClientAddressLength(httpADT_t s, socklen_t addressLength) {
	s->clientAddrLen = addressLength;
}

buffer *getReadBuffer(httpADT_t s) {
	return &(s->readBuffer);
}

buffer *getWriteBuffer(httpADT_t s) {
	return &(s->writeBuffer);
}

buffer *getRequestLineBuffer(httpADT_t s) {
	return &(s->requestLine);
}

buffer *getResponseLineBuffer(httpADT_t s) {
	return &(s->responseLine);
}

buffer *getFinishParserBuffer(httpADT_t s) {
	return &(s->finishParserBuffer);
}

buffer *getErrorBuffer(httpADT_t s) {
	return &(s->errorBuffer);
}

void setErrorBuffer(httpADT_t s, uint8_t *data, int length) {
	buffer_init(&s->errorBuffer, length, data);
}

void setRequestMethod(httpADT_t s, unsigned method) {
	s->requestMethod = method;
}

unsigned getRequestMethod(httpADT_t s) {
	return s->requestMethod;
}

char *getOriginHost(struct http *s) {
	return s->host;
}

void setOriginHost(struct http *s, char *requestHost) {
	s->host = requestHost;
}

void setErrorType(struct http *s, int errorTypeFound) {
	s->errorTypeFound = errorTypeFound;
}

int getTransformContent(struct http *s) {
	return s->transformContent;
}

void setTransformContent(struct http *s, int transformContent) {
	s->transformContent = transformContent;
}

MediaRangePtr_t getMediaRangeHTTP(struct http *s) {
	return s->mediaRanges;
}

int getErrorType(struct http *s) {
	return s->errorTypeFound;
}

void incrementReferences(struct http *s) {
	s->references = s->references + 1;
}

// Pool of struct http, to be reused.
static const unsigned maxPool = MAX_POOL_SIZE;
static unsigned poolSize	  = 0; // current size
static struct http *pool	  = 0;

static const struct state_definition clientStatbl[] = {
	{
		.state		   = PARSE,
		.on_arrival	= parseInit,
		.on_read_ready = parseRead,
		.on_departure  = parseDestroy,
	},
	{
		.state = CONNECT_TO_ORIGIN,
		// .on_arrival       = copy_init,
		.on_block_ready = addressResolvNameDone,
		// .on_write_ready   = copy_w,
	},

	{
		.state			= HANDLE_REQUEST,
		.on_arrival		= requestInit,
		.on_read_ready  = requestRead,
		.on_write_ready = requestWrite,
	},
	{
		.state			= HANDLE_RESPONSE,
		.on_arrival		= responseInit,
		.on_read_ready  = responseRead,
		.on_write_ready = responseWrite,
		.on_departure   = responceDestroy,
	},
	{
		.state			= HANDLE_RESPONSE_WITH_TRANSFORMATION,
		.on_arrival		= responseWithTransformInit,
		.on_read_ready  = responseWithTransformRead,
		.on_write_ready = responseWithTransformWrite,
	},
	{
		.state			= TRANSFORM_BODY,
		.on_arrival		= transformBodyInit,
		.on_read_ready  = transformBodyRead,
		.on_write_ready = transformBodyWrite,
		.on_departure   = transformBodyDestroy,
	},
	{
		.state			= ERROR_CLIENT,
		.on_arrival		= errorInit,
		.on_write_ready = errorHandleWrite,
	},
	{
		.state = DONE,

	},
	{
		.state = ERROR,
	}};

static const struct state_definition *httpDescribeStates(void) {
	return clientStatbl;
}

struct http *httpNew(int clientFd) {
	struct http *ret;

	if (pool == NULL) {
		ret = malloc(sizeof(*ret));
	}
	else {
		ret		  = pool;
		pool	  = pool->next;
		ret->next = 0;
	}

	if (ret == NULL) {
		goto finally;
	}

	memset(ret, 0x00, sizeof(*ret));

	ret->host			  = NULL;
	ret->originPort		  = 80;
	ret->originFd		  = -1;
	ret->clientFd		  = clientFd;
	ret->clientAddrLen	= sizeof(ret->clientAddr);
	ret->transformContent = FALSE;
	ret->mediaRanges =
		createMediaRangeFromListOfMediaType(getMediaRange(getConfiguration()));

	// setting state machine
	ret->stm.initial   = PARSE_METHOD;
	ret->stm.max_state = ERROR;

	ret->stm.states = httpDescribeStates();
	stm_init(&ret->stm);

	buffer_init(&ret->readBuffer, SIZE_OF_ARRAY(ret->rawBuffA), ret->rawBuffA);
	buffer_init(&ret->writeBuffer, SIZE_OF_ARRAY(ret->rawBuffB), ret->rawBuffB);
	buffer_init(&ret->finishParserBuffer, SIZE_OF_ARRAY(ret->finishParserData),
				ret->finishParserData);
	buffer_init(&ret->requestLine, SIZE_OF_ARRAY(ret->raWRequest),
				ret->raWRequest);
	buffer_init(&ret->responseLine, SIZE_OF_ARRAY(ret->rawResponse),
				ret->rawResponse);

	ret->errorTypeFound = DEFAULT;

	ret->references = 1;
finally:
	return ret;
}

void httpDestroyData(struct http *s) {
	if (s->originResolution != NULL) {
		freeaddrinfo(s->originResolution);
		s->originResolution = 0;
	}

	freeMediaRange(s->mediaRanges);
	free(s);
}

void httpDestroy(struct http *s) {
	if (s != NULL) {
		if (s->references == 1) {
			if (poolSize < maxPool) {
				s->next = pool;
				pool	= s;
				poolSize++;
			}
			else {
				httpDestroyData(s);
			}
		}
		else {
			s->references -= 1;
		}
	}
}

void httpPoolDestroy(void) {
	struct http *next, *s;

	for (s = pool; s != NULL; s = next) {
		next = s->next;
		free(s);
	}
}

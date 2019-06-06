#include <http.h>
#include <httpProxyADT.h>
#include <connectToOrigin.h>
#include <handleRequest.h>
#include <handleResponse.h>
#include <handleResponseWithTransform.h>
#include <headersParser.h>

static const struct state_definition *httpDescribeStates(void);

struct http {
	// Client info
	struct sockaddr_storage clientAddr;
	socklen_t clientAddrLen;
	int clientFd;

	// Origin server address resolution
	struct addrinfo *originResolution;
	// Current try origin server address
	struct addrinfo *originResolutionCurrent;

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

	// Client States
	union {
		struct parseRequest parseRequest;
		struct handleRequest handleRequest;
		struct handleResponse handleResponse;
		struct handleResponseWithTransform handleResponseWithTransform;
		int other;
		// struct request_st         request;
		// struct copy               copy;
	} clientState;
	// /** estados para el origin_fd */
	// union {
	//     struct connecting         conn;
	//     struct copy               copy;
	// } orig;

	// buffers to use: readBuffer and writeBuffer.

	uint8_t rawBuffA[BUFFER_SIZE], rawBuffB[BUFFER_SIZE];
	buffer readBuffer, writeBuffer;

	// Number of references to this object, if one destroy
	unsigned references;

	// Next in pool
	struct http *next;
};

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
	},
	{
		.state			= HANDLE_RESPONSE_WITH_TRANSFORMATION,
		.on_arrival		= responseWithTransformInit,
		.on_read_ready  = responseWithTransformRead,
		.on_write_ready = responseWithTransformWrite,
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

	ret->host		   = NULL;
	ret->originFd	  = -1;
	ret->clientFd	  = clientFd;
	ret->clientAddrLen = sizeof(ret->clientAddr);

	// setting state machine
	ret->stm.initial   = PARSE_METHOD;
	ret->stm.max_state = ERROR;

	ret->stm.states = httpDescribeStates();
	stm_init(&ret->stm);

	buffer_init(&ret->readBuffer, SIZE_OF_ARRAY(ret->rawBuffA), ret->rawBuffA);
	buffer_init(&ret->writeBuffer, SIZE_OF_ARRAY(ret->rawBuffB), ret->rawBuffB);

	ret->references = 1;
finally:
	return ret;
}

void httpDestroyData(struct http *s) {
	if (s->originResolution != NULL) {
		freeaddrinfo(s->originResolution);
		s->originResolution = 0;
	}

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

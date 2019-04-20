#include <http.h>
#include <httpProxyADT.h>

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

	// HTTP proxy state machine
	struct state_machine stm;

	// /** estados para el client_fd */
	// union {
	//     struct hello_st           hello;
	//     struct request_st         request;
	//     struct copy               copy;
	// } client;
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

struct sockaddr_storage *getClientAddress(httpADT s) {
	return &(s->clientAddr);
}

socklen_t getClientAddressLength(httpADT s) {
	return s->clientAddrLen;
}

void setClientAddressLength(httpADT s, socklen_t addressLength) {
	s->clientAddrLen = addressLength;
}

buffer *getReadBuffer(httpADT s) {
	return &(s->readBuffer);
}

buffer *getWriteBuffer(httpADT s) {
	return &(s->writeBuffer);
}

// Pool of struct http, to be reused.
static const unsigned maxPool = MAX_POOL_SIZE;
static unsigned poolSize	  = 0; // current size
static struct http *pool	  = 0;

static const struct state_definition clientStatbl[] = {
	// {
	//     .state            = HELLO_READ,
	//     .on_arrival       = hello_read_init,
	//     .on_departure     = hello_read_close,
	//     .on_read_ready    = hello_read,
	// }, {
	{
		.state = COPY,
		// .on_arrival       = copy_init,
		.on_read_ready = printRead,
		// .on_write_ready   = copy_w,
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

	ret->originFd	  = -1;
	ret->clientFd	  = clientFd;
	ret->clientAddrLen = sizeof(ret->clientAddr);

	// setting state machine
	ret->stm.initial   = COPY;
	ret->stm.max_state = ERROR;
	ret->stm.states	= httpDescribeStates();
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

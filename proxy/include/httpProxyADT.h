#ifndef PROXY_HTTP_ADT_H
#define PROXY_HTTP_ADT_H

#include <netdb.h>
#include <stdlib.h>
#include <stm.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <buffer.h>
#include <methodParser.h>
#include <targetParser.h>
#include <versionParser.h>
#include <hostHeaderParser.h>
#include <handleParsers.h>
#include <handleError.h>
#include <mediaRange.h>
#include <configuration.h>
#include <metric.h>

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_POOL_SIZE 50
#define BUFFER_SIZE 20
#define MAX_PARSER 1000 // TODO: chech that number
#define MAX_FIRST_LINE_LENGTH 2048

typedef struct http *httpADT_t;

enum httpState {
	/**
	 * Reads first line of request message and finds host if it is there
	 * If the host is not in the first line it continue to analyse the headers
	 *
	 * Interests:
	 *   ClientFd
	 *      - OP_READ       Until client sends end of line and has found host.
	 *
	 * Transitions:
	 *   - REQUEST_CONNECT  If is a valid method and host.
	 *
	 *   - ERROR            If method not supported.
	 *
	 */
	PARSE,
	/*
	 * Resolves address and connects to origin
	 */
	CONNECT_TO_ORIGIN,

	/**
	 * Copia bytes entre client_fd y origin_fd.
	 *
	 * Intereses: (tanto para client_fd y origin_fd)
	 *   - OP_READ  si hay espacio para escribir en el buffer de lectura
	 *   - OP_WRITE si hay bytes para leer en el buffer de escritura
	 *
	 * Transicion:
	 *   - DONE     cuando no queda nada mas por copiar.
	 */
	HANDLE_REQUEST,
	HANDLE_RESPONSE,
	HANDLE_RESPONSE_WITH_TRANSFORMATION,
	TRANSFORM_BODY,
	ERROR_CLIENT,

	// final states
	DONE,
	ERROR,
};

enum parserState { PARSE_METHOD, PARSE_TARGET, PARSE_VERSION, PARSE_HEADER };

// structure for parse request state
struct parseRequest {
	buffer *input;
	buffer *finishParserBuffer;
	enum parserState state;
	struct methodParser methodParser;
	struct targetParser targetParser;
	struct versionParser versionParser;
	struct headerParser headerParser;
};

/*
 * Creates a new struct http
 */
httpADT_t httpNew(int clientFd);

/*
 * Efectively destroys a struct http
 */
void httpDestroyData(httpADT_t s);

/*
 * Destroys a struct http, takes into account references and the pool
 */
void httpDestroy(httpADT_t s);

/*
 * Cleans the pool
 */
void httpPoolDestroy(void);

/*
 * Returns state machine
 */
struct state_machine *getStateMachine(httpADT_t s);

/*
 * Returns client fd
 */
int getClientFd(httpADT_t s);

/*
 * Returns origin server fd
 */
int getOriginFd(httpADT_t s);

/*
 * Sets origin server fd to originFd
 */
void setOriginFd(struct http *s, int originFd);

/*
 * Returns client address
 */
struct sockaddr_storage *getClientAddress(httpADT_t s);

/*
 * Returns client address Length
 */
socklen_t getClientAddressLength(httpADT_t s);

/*
 * Sets client address Length
 */
void setClientAddressLength(httpADT_t s, socklen_t addressLength);

/*
 * Returns read buffer
 */
buffer *getReadBuffer(httpADT_t s);

/*
 * Returns write buffer
 */
buffer *getWriteBuffer(httpADT_t s);

/*
 * Returns requestLine buffer
 */
buffer *getRequestLineBuffer(httpADT_t s);

/*
 * Returns responseLine buffer
 */
buffer *getResponseLineBuffer(httpADT_t s);

/*
 * Returns finish parser buffer
 */
buffer *getFinishParserBuffer(httpADT_t s);

/*
 * Returns http parse request structure
 */
struct parseRequest *getParseRequestState(httpADT_t s);

/*
 * Returns http handle request structure
 */
struct handleRequest *getHandleRequestState(httpADT_t s);

/*
 * Returns http handle response structure
 */
struct handleResponse *getHandleResponseState(httpADT_t s);

/*
 * Returns http handle response with transform structure
 */
struct handleResponseWithTransform *
getHandleResponseWithTransformState(httpADT_t s);

/*
 * Returns transform body structure
 */

struct transformBody *getTransformBodyState(httpADT_t s);

/*
 * Sets http request method
 */
void setRequestMethod(httpADT_t s, unsigned method);

/*
 * Returns http request method
 */
unsigned getRequestMethod(httpADT_t s);

/*
 * Returns origin server port
 */
unsigned short getOriginPort(struct http *s);

/*
 * Sets origin server port
 */
void setOriginPort(struct http *s, unsigned short originPort);

/*
 * Return origin server host
 */
char *getOriginHost(struct http *s);

/*
 * Sets origin server host
 */
void setOriginHost(struct http *s, char *requestHost);

/*
 * Increments references to struct http in 1
 */
void incrementReferences(struct http *s);

/*
 * Return error buffer
 */
buffer *getErrorBuffer(httpADT_t s);

/*
 * Set data as errors buffers data
 */
void setErrorBuffer(httpADT_t s, uint8_t *data, int length);

/*
 * Set type of error found
 */
void setErrorType(struct http *s, int errorTypeFound);

/*
 * Return error type
 */
int getErrorType(struct http *s);

// TODO
struct addrinfo *getOriginResolutions(struct http *s);
void setOriginResolutions(struct http *s, struct addrinfo *originResolution);
int getTransformContent(struct http *s);
void setTransformContent(struct http *s, int transformContent);
MediaRangePtr_t getMediaRangeHTTP(struct http *s);

#endif

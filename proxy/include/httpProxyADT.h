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
#define BUFFER_SIZE 4000
#define MAX_PARSER 1000
#define MAX_FIRST_LINE_LENGTH 2048

typedef struct http *httpADT_t;

enum httpState {
	/**
	 * Reads first line of request message and finds host if it is there
	 * If the host is not in the first line it continue to analyse the headers
	 *
	 * Interests:
	 *
	 *   ClientFd:
	 *
	 *      - OP_READ       Until client sends end of line and has found host.
	 *
	 * Transitions:
	 *
	 *   - CONNECT_TO_ORIGIN    If is a valid method and host.
	 *
	 *   - ERROR_CLIENT         If any error to be send to the client
	 *
	 *   - ERROR                Any other error.
	 *
	 */
	PARSE,

	/*
	 * Resolves address and connects to origin
	 *
	 * Transitions:
	 *
	 *  - HANDLE_REQUEST        If it can connecto correctly to origin
	 *                          server.
	 *
	 *  - ERROR_CLIENT          If there is any problem in address resolution
	 *                          or in connecting to origin.
	 *
	 *  - ERROR                 If any other error occurs.
	 */
	CONNECT_TO_ORIGIN,

	/**
	 * Parse requests headers, filters some of them and modify its value and
	 * send them to origin.
	 * On this state request headers are read from a buffer and parsed request
	 * headers are stored on another buffer.
	 * If request includes body it will be transfered as it is read.
	 *
	 * Interests:
	 *
	 *  ClientFd:
	 *      - OP_READ           If it can read from client.
	 *
	 *  ServerFd
	 *      - OP_READ           Always waiting for server response.
	 *
	 *      - OP_WRITE          If there is something to send to origin.
	 *
	 * Transitions:
	 *
	 *   - HANDLE_RESPONSE      If something has been read from server.
	 *
	 *   - ERROR                If an error occurs.
	 */
	HANDLE_REQUEST,

	/**
	 * Parse response headers, filters some of them and modify its value and
	 * send them to origin.
	 * If transformation is disabled would also transfer body as it is received.
	 * This state use one buffer to read response headers and another buffer to
	 * store parsed response headers. Response body would be transferred as it
	 * is received. It uses also a third buffer to read extra info sent from
	 * client to origin.
	 *
	 * Interests:
	 *
	 *  ClientFd:
	 *      - OP_READ           If it can read from client.
	 *
	 *      - OP_WRITE          If it has something to write to client
	 *
	 *  ServerFd:
	 *      - OP_READ           If it has space to read response and origin
	 *                          has not closed connection.
	 *
	 *      - OP_WRITE          If there is something to write to client.
	 *
	 * Transitions:
	 *
	 *   - TRANSFORM_BODY       If transformations are enabled and all response
	 *                          headers has been read.
	 *
	 *   - DONE                 If it has finished sending response from
	 *                          origin to client and transformations are
	 *                          disabled.
	 *
	 *   - ERROR                If an error occurs.
	 */
	HANDLE_RESPONSE,

	/**
	 * This state is called when all headers are read from response and
	 * transformations are enabled. It has three different modes of working 1-
	 * When transformations are enabled and mime types does not match filter, or
	 * when they match but program fails. In this mode it will send body
	 *     received body chunked.
	 *  2- When mime types match filter and command is executed correctly. In
	 *     this mode response read from origin will be sent to the transformer
	 *     command and its response will be chunked and sent to client.
	 *  3- When mime type does not match filter and response is sent chunked
	 * from origin or when content-encoding is set and value differs from
	 * identity. In this mode bytes will be sent as they are read from origin.
	 *
	 * Interests:
	 *
	 *  ClientFd:
	 *      - OP_WRITE          If it has something to write to client.
	 *
	 *  ServerFd:
	 *      - OP_READ           If it has space to read response and origin
	 *                          has not closed connection.
	 *  WriteToTransformFd:
	 *      - OP_WRITE          If it has something to write to transform
	 * command.
	 *
	 *  ReadFromTransformFd:
	 *      - OP_READ           If it has space to read transform command
	 * response.
	 *
	 *
	 * Transitions:
	 *
	 *   - DONE                 When it finishes sendind response to client.
	 *
	 *   - ERROR                If an error occurs.
	 */
	TRANSFORM_BODY,

	/**
	 * Send the error message that corespond to the erro code set in the http
	 * structure.
	 *
	 * Interests:
	 *
	 *   ClientFd:
	 *
	 *      - OP_WRITE          Until it sends all the error.
	 *
	 * Transitions:
	 *
	 *   - ERROR                Any error or finish correctly (because it is
	 *							suppose that a other error happend in a previous
	 *							state).
	 */
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

/*
 * Returns isChunked from http structure
 */
uint8_t getIsChunked(struct http *s);

/*
 *  Sets isChunked with received value
 */
void setIsChunked(struct http *s, uint8_t isChunked);

/*
 *	Returns the set of resolutions of the origin found in the DNS query or null
 */
struct addrinfo *getOriginResolutions(struct http *s);

/*
 *	Sets the oring host resolutions
 */
void setOriginResolutions(struct http *s, struct addrinfo *originResolution);

/*
 * Returns 1 if there must be transformation or 0 otherwise
 */
int getTransformContent(struct http *s);

/*
 * Set whenever must be transformation
 */
void setTransformContent(struct http *s, int transformContent);

/*
 * Return the media range that is compare with the content length in
 * order to determine if there is transformations
 */
MediaRangePtr_t getMediaRangeHTTP(struct http *s);

/*
 * Set selector copy that is given to the child process in order to free it
 */
void setSelectorCopy(struct http *s, void **selectorCopyForOtherThread);

/*
 * Get selector copy
 */
void **getSelectorCopy(struct http *s);

#endif

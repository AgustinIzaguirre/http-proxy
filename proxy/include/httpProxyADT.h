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

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_POOL_SIZE 50
#define BUFFER_SIZE 2048

typedef struct http *httpADT_t;

enum httpState {
	/**
	 * Reads first line of request message and ignores starting CRLF
	 * for robustness.
	 *
	 * Interests:
	 *   ClientFd
	 *      - OP_READ       Until client sends first line.
	 *
	 * Transitions:
	 *   - REQUEST_CONNECT  If is a valid method and host.
	 *
	 *   - ERROR            If method not supported.
	 *
	 */
	PARSE_METHOD,
	PARSE_TARGET, // TODO: comentarios
	PARSE_HOST,
	RESOLV_NAME,
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
	COPY,

	// final states
	DONE,
	ERROR,
};

// structure for parse request state
struct parseRequest {
	buffer *input;
	struct methodParser methodParser;
	struct targetParser targetParser;
	// struct hostParser   hostParser;
	//    char                *host;
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
 * Returns http parse request structure
 */
struct parseRequest *getParseRequestState(httpADT_t s);

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

#endif

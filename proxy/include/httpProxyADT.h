#ifndef PROXY_HTTP_ADT_H
#define PROXY_HTTP_ADT_H

#include <buffer.h>
#include <netdb.h>
#include <stdlib.h>
#include <stm.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_POOL_SIZE 50
#define BUFFER_SIZE 2048

typedef struct http *httpADT;

enum httpState {

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

/*
 * Creates a new struct http
 */
httpADT httpNew(int clientFd);

/*
 * Efectively destroys a struct http
 */
void httpDestroyData(httpADT s);

/*
 * Destroys a struct http, takes into account references and the pool
 */
void httpDestroy(httpADT s);

/*
 * Cleans the pool
 */
void httpPoolDestroy(void);

/*
 * Returns state machine
 */
struct state_machine *getStateMachine(httpADT s);

/*
 * Returns client fd
 */
int getClientFd(httpADT s);

/*
 * Returns origin server fd
 */
int getOriginFd(httpADT s);

/*
 * Returns client address
 */
struct sockaddr_storage *getClientAddress(httpADT s);

/*
 * Returns client address Length
 */
socklen_t getClientAddressLength(httpADT s);

/*
 * Sets client address Length
 */
void setClientAddressLength(httpADT s, socklen_t addressLength);

/*
 * Returns read buffer
 */
buffer *getReadBuffer(httpADT s);

/*
 * Returns write buffer
 */
buffer *getWriteBuffer(httpADT s);

#endif
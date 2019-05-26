#include <http.h>
#include <httpProxyADT.h>
#include <selector.h>
#include <stm.h>
#include <configuration.h>
#include <connectToOrigin.h>
#include <handleRequest.h>

#include <assert.h> // assert
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include <time.h>
#include <unistd.h> // close

#include <arpa/inet.h>

static void httpRead(struct selector_key *key);
static void httpWrite(struct selector_key *key);
static void httpDone(struct selector_key *key);
static void httpClose(struct selector_key *key);
static void httpBlock(struct selector_key *key);

static const struct fd_handler httpHandler = {
	.handle_read  = httpRead,
	.handle_write = httpWrite,
	.handle_close = httpClose,
	.handle_block = httpBlock,
};

const struct fd_handler *getHttpHandler() {
	return &httpHandler;
}

void httpPassiveAccept(struct selector_key *key) {
	printf("conexion aceptada\n"); // evans
	struct sockaddr_storage clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	struct http *state		= NULL;
	const int client =
		accept(key->fd, (struct sockaddr *) &clientAddr, &clientAddrLen);

	if (client == -1) {
		goto fail;
	}

	if (selector_fd_set_nio(client) == -1) {
		goto fail;
	}

	state = httpNew(client);

	if (state == NULL) {
		// sin un estado, nos es imposible manejaro.
		// tal vez deberiamos apagar accept() hasta que detectemos
		// que se liberó alguna conexión.
		goto fail;
	}

	memcpy(getClientAddress(state), &clientAddr, clientAddrLen);
	setClientAddressLength(state, clientAddrLen);

	if (SELECTOR_SUCCESS !=
		selector_register(key->s, client, &httpHandler, OP_READ, state)) {
		goto fail;
	}

	return;

fail:

	if (client != -1) {
		close(client);
	}

	httpDestroy(state);
}

static void httpRead(struct selector_key *key) {
	struct state_machine *stm = getStateMachine(GET_DATA(key));
	// struct state_machine *stm   = &(GET_DATA(key)->stm); without ADT
	const enum httpState st = stm_handler_read(stm, key);

	if (ERROR == st || DONE == st) {
		httpDone(key);
	}
}

static void httpWrite(struct selector_key *key) {
	struct state_machine *stm = getStateMachine(GET_DATA(key));
	printf("Se desperto para escribir, porque no llama?\n"); // evans
	const enum httpState st = stm_handler_write(stm, key);

	if (ERROR == st || DONE == st) {
		httpDone(key);
	}
}

static void httpBlock(struct selector_key *key) {
	struct state_machine *stm = getStateMachine(GET_DATA(key));
	const enum httpState st   = stm_handler_block(stm, key);

	if (ERROR == st || DONE == st) {
		httpDone(key);
	}
}

static void httpClose(struct selector_key *key) {
	httpDestroy(GET_DATA(key));
}

static void httpDone(struct selector_key *key) {
	const int fds[] = {
		getClientFd(GET_DATA(key)), getOriginFd(GET_DATA(key)),
		// GET_DATA(key)->clientFd, without ADT
		// GET_DATA(key)->originFd, without ADT
	};

	for (unsigned i = 0; i < SIZE_OF_ARRAY(fds); i++) {
		if (fds[i] != -1) {
			if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i])) {
				abort();
			}
			close(fds[i]);
		}
	}
}

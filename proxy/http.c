#include <http.h>
#include <httpProxyADT.h>
#include <selector.h>
#include <stm.h>
#include <configuration.h>
#include <connectToOrigin.h>
#include <handleRequest.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
	const enum httpState st   = stm_handler_read(stm, key);

	if (ERROR == st || DONE == st) {
		httpDone(key);
	}
}

static void httpWrite(struct selector_key *key) {
	struct state_machine *stm = getStateMachine(GET_DATA(key));
	const enum httpState st   = stm_handler_write(stm, key);

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
		getClientFd(GET_DATA(key)),
		getOriginFd(GET_DATA(key)),
	};

	decreaseConcurrentConections();

	if (getSelectorCopy(GET_DATA(key)) != NULL) {
		void **aux = getSelectorCopy(GET_DATA(key));
		free(aux[1]);
		free(aux);
		setSelectorCopy(GET_DATA(key), NULL);
	}

	if (getOriginHost(GET_DATA(key)) != NULL) {
		free(getOriginHost(GET_DATA(key)));
		setOriginHost(GET_DATA(key), NULL);
	}

	if (getMediaRangeHTTP(GET_DATA(key)) != NULL) {
		freeMediaRange(getMediaRangeHTTP(GET_DATA(key)));
	}

	if (getOriginResolutions((GET_DATA(key))) != NULL) {
		freeaddrinfo(getOriginResolutions((GET_DATA(key))));
		setOriginResolutions(GET_DATA(key), NULL);
	}

	for (unsigned i = 0; i < SIZE_OF_ARRAY(fds); i++) {
		if (fds[i] != -1) {
			if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i])) {
				abort();
			}
			close(fds[i]);
		}
	}
}

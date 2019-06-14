#include <management.h>

static const char *errorMessage = "";

int listenManagementSocket(int managementSocket, size_t backlogQuantity) {
	if (listen(managementSocket, backlogQuantity) < 0) {
		errorMessage = "Unable to listen";
		return -1;
	}

	if (selector_fd_set_nio(managementSocket) == -1) { // TODO: maybe < 0?
		errorMessage = "Getting server socket flags";
		return -1;
	}

	return 0;
}

const char *getManagementErrorMessage() {
	return errorMessage;
}

void managementPassiveAccept(struct selector_key *key) {
	// 	struct sockaddr_storage clientAddress;
	// 	socklen_t clientAddressLen = sizeof(clientAddress);
	// 	struct http *state = NULL;
	// 	const int client =
	// 		accept(key->fd, (struct sockaddr *) &clientAddress,
	// &clientAddressLen);
	//
	// 	if (client == -1) {
	// 		goto fail;
	// 	}
	//
	// 	if (selector_fd_set_nio(client) == -1) {
	// 		goto fail;
	// 	}
	//
	// 	state = httpNew(client);
	//
	// 	if (state == NULL) {
	// 		// sin un estado, nos es imposible manejaro.
	// 		// tal vez deberiamos apagar accept() hasta que detectemos
	// 		// que se liberó alguna conexión.
	// 		goto fail;
	// 	}
	//
	// 	memcpy(getClientAddress(state), &clientAddress, clientAddressLen);
	// 	setClientAddressLength(state, clientAddressLen);
	//
	// 	if (SELECTOR_SUCCESS !=
	// 		selector_register(key->s, client, &httpHandler, OP_READ, state)) {
	// 		goto fail;
	// 	}
	//
	// 	return;
	//
	// fail:
	//
	// 	if (client != -1) {
	// 		close(client);
	// 	}
	//
	// 	httpDestroy(state);
}

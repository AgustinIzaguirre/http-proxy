#include <management.h>

static void handleRead(struct selector_key *key);
static void handleWrite(struct selector_key *key);
static void handleClose(struct selector_key *key);
static manager_t *newManager();
static int handleAuthenticatedRead(struct selector_key *key);
static int handleNonAuthenticatedRead(struct selector_key *key);
static uint8_t authenticate(char *username, char *password);
static void initializeTimeTags();
static void manageGetRequest(manager_t *client);
static void manageSetRequest(manager_t *client);
static uint8_t isValidGetId(ids_t id);
static uint8_t isValidSetId(ids_t id);

static const char *errorMessage = "";

timeTag_t timeTags[ID_QUANTITY];

static const struct fd_handler managementHandler = {.handle_read  = handleRead,
													.handle_write = handleWrite,
													.handle_close = handleClose,
													.handle_block = NULL};

static manager_t *newManager() {
	manager_t *manager = calloc(sizeof(manager_t), 1);

	manager->isAuthenticated = FALSE;

	return manager;
}

int listenManagementSocket(int managementSocket, size_t backlogQuantity) {
	if (listen(managementSocket, backlogQuantity) < 0) {
		errorMessage = "Unable to listen";
		return -1;
	}

	if (selector_fd_set_nio(managementSocket) == -1) { // TODO: maybe < 0?
		errorMessage = "Getting server socket flags";
		return -1;
	}

	initializeTimeTags();

	return 0;
}

const char *getManagementErrorMessage() {
	return errorMessage;
}

static void initializeTimeTags() {
	time_t initialTimeTag = time(NULL);

	for (int i = 0; i < ID_QUANTITY; i++) {
		timeTags[i] = initialTimeTag;
	}
}

void managementPassiveAccept(struct selector_key *key) {
	struct sockaddr_storage clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);

	const int fd = accept(key->fd, (struct sockaddr *) &clientAddress,
						  &clientAddressLength);

	if (fd == -1) {
		goto fail;
	}

	if (selector_fd_set_nio(fd) == -1) {
		goto fail;
	}

	// "Management connection accepted\n" TODO: logger?

	manager_t *client = newManager();

	selector_status selectorStatus =
		selector_register(key->s, fd, &managementHandler, OP_READ, client);

	if (selectorStatus != SELECTOR_SUCCESS) {
		goto fail;
	}

	return;

fail:
	if (fd != -1) {
		close(fd);
	}
}

static void handleRead(struct selector_key *key) {
	printf("Handling Read\n"); // TODO
	manager_t *client = (manager_t *) key->data;
	int read;

	if (client->isAuthenticated) {
		read = handleAuthenticatedRead(key);
	}
	else {
		read = handleNonAuthenticatedRead(key);
	}

	if (read < 0) {
		// TODO: destroy if fails?
	}

	selector_set_interest(key->s, key->fd, OP_WRITE);
}

static int handleAuthenticatedRead(struct selector_key *key) {
	printf("Handling auth Read\n"); // TODO
	manager_t *client = (manager_t *) key->data;

	int read = recvRequest(key->fd, &client->request);

	if (read < 0) {
		errorMessage = getProtocolErrorMessage();
		// TODO: destroy?
	}

	client->response.streamNumber = client->request.streamNumber;

	client->response.status.generalStatus   = OK_STATUS;
	client->response.status.operationStatus = OK_STATUS;

	switch (client->request.operation) {
		case BYE_OP:
			// TODO: manage BYE
			break;
		case GET_OP:
			manageGetRequest(client);
			break;
		case SET_OP:
			manageSetRequest(client);
			break;
		default:
			/* Invalid OPERATION */
			client->response.status.generalStatus   = ERROR_STATUS;
			client->response.status.operationStatus = ERROR_STATUS;
	}

	return read;
}

static void manageGetRequest(manager_t *client) {
	ids_t id = client->request.id;

	if (!isValidGetId(id)) {
		/* Invalid ID */
		client->response.status.generalStatus = ERROR_STATUS;
		client->response.status.idStatus	  = ERROR_STATUS;
		return;
	}

	if (client->request.timeTag == timeTags[id]) {
		/* Valid ID & Same TIME-TAG: Response without data */
		client->response.status.timeTagStatus = OK_STATUS;
		return;
	}

	/* Valid ID & Different TIME-TAG: Response with data */
	client->response.status.generalStatus = ERROR_STATUS;
	client->response.status.timeTagStatus = ERROR_STATUS;

	switch (id) {
		case MIME_ID:
			break;
		case BF_ID:
			break;
		case CMD_ID:
			break;
		case MTR_CN_ID:
			break;
		case MTR_HS_ID:
			break;
		case MTR_BT_ID:
			break;
		case MTR_ID:
			break;
		case TF_ID:
			break;
	}
}

static uint8_t isValidGetId(ids_t id) {
	return id >= MIME_ID && id <= TF_ID;
}

static uint8_t isValidSetId(ids_t id) {
	return id == MIME_ID || id == BF_ID || id == CMD_ID || id == TF_ID;
}

static void manageSetRequest(manager_t *client) {
	ids_t id = client->request.id;

	if (!isValidSetId(id)) {
		/* Invalid ID */
		client->response.status.generalStatus = ERROR_STATUS;
		client->response.status.idStatus	  = ERROR_STATUS;
		return;
	}

	if (client->request.timeTag == timeTags[id]) {
		/* Valid ID & Same TIME-TAG: Can set (override) resource */
		client->response.status.timeTagStatus = OK_STATUS;

		switch (id) {
			case MIME_ID:
				break;
			case BF_ID:
				break;
			case CMD_ID:
				break;
			case TF_ID:
				break;
		}

		return;
	}

	/* Valid ID & Different TIME-TAG: Response with data */
	client->response.status.generalStatus = ERROR_STATUS;
	client->response.status.timeTagStatus = ERROR_STATUS;
}

static int handleNonAuthenticatedRead(struct selector_key *key) {
	printf("Handling non auth Read\n"); // TODO

	manager_t *client = (manager_t *) key->data;
	char *username;
	char *password;
	uint8_t hasSameVersion;

	int read = recvAuthenticationRequest(key->fd, &username, &password,
										 &hasSameVersion);

	if (read < 0) {
		errorMessage = getProtocolErrorMessage();
		// TODO: destroy?
	}

	if (hasSameVersion) {
		client->authResponse.status.generalStatus = OK_STATUS;
		client->authResponse.status.versionStatus = OK_STATUS;

		if (authenticate(username, password)) {
			client->isAuthenticated							 = TRUE;
			client->authResponse.status.authenticationStatus = OK_STATUS;
		}
		else {
			client->isAuthenticated							 = FALSE;
			client->authResponse.status.generalStatus		 = ERROR_STATUS;
			client->authResponse.status.authenticationStatus = ERROR_STATUS;
		}
	}
	else {
		client->authResponse.status.generalStatus = ERROR_STATUS;
		client->authResponse.status.versionStatus = ERROR_STATUS;
	}

	return read;
}

static uint8_t authenticate(char *username, char *password) {
	if (strcmp(USERNAME, username) == 0 && strcmp(PASSWORD, password) == 0) {
		return TRUE;
	}

	return FALSE;
}

static void handleWrite(struct selector_key *key) {
	printf("Handling Write\n"); // TODO

	manager_t *client = (manager_t *) key->data;
	int sent;

	if (client->isAuthenticated) {
		printf("Is auth!\n"); // TODO
		sent = sendResponse(key->fd, client->response);
	}
	else {
		printf("Not auth yet!\n"); // TODO
		sent = sendAuthenticationResponse(key->fd, client->authResponse);
	}

	if (sent < 0) {
		errorMessage = getProtocolErrorMessage();
		// TODO: destroy?
	}

	selector_set_interest(key->s, key->fd, OP_READ);
}

static void handleClose(struct selector_key *key) {
	// TODO: hmmm? what we need to do here?
}

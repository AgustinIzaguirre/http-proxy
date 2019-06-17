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
static uint8_t isValidGetId(resId_t id);
static uint8_t isValidSetId(resId_t id);
static void manageGetCommandRequest(response_t *response);
static void manageGetMetricRequest(resId_t id, response_t *response);
static void manageGetTransformationStatusRequest(response_t *response);
static void manageGetMediaRangeRequest(response_t *response);

static const char *errorMessage = "";

timeTag_t timeTags[ID_QUANTITY];

static const struct fd_handler managementHandler = {.handle_read  = handleRead,
													.handle_write = handleWrite,
													.handle_close = handleClose,
													.handle_block = NULL};

timeTag_t generateAndUpdateTimeTag(uint8_t id) {
	timeTag_t timeTag = time(NULL);

	timeTags[id] = timeTag;

	return timeTag;
}

static manager_t *newManager() {
	manager_t *manager = calloc(sizeof(manager_t), 1);

	manager->isAuthenticated  = FALSE;
	manager->authResponseSent = FALSE;

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
	timeTag_t initialTimeTag = time(NULL);

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
	manager_t *client = (manager_t *) key->data;
	int read;

	if (client->isAuthenticated) {
		read = handleAuthenticatedRead(key);
	}
	else {
		read = handleNonAuthenticatedRead(key);
	}

	if (read <= 0) {
		// TODO: destroy?
		selector_unregister_fd(key->s, key->fd);
	}
	else {
		selector_set_interest(key->s, key->fd, OP_WRITE);
	}
}

static int handleAuthenticatedRead(struct selector_key *key) {
	manager_t *client = (manager_t *) key->data;

	int read = recvRequest(key->fd, &client->request);

	if (read <= 0) {
		errorMessage = getProtocolErrorMessage();
		// TODO: destroy?
		return read;
	}

	client->response.streamNumber = client->request.streamNumber;

	client->response.operation = client->request.operation;
	client->response.id		   = client->request.id;

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
	resId_t id = client->request.id;

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
			manageGetMediaRangeRequest(&client->response);
			break;
		case CMD_ID:
			manageGetCommandRequest(&client->response);
			break;
		case MTR_CN_ID:
		case MTR_HS_ID:
		case MTR_BT_ID:
			manageGetMetricRequest(id, &client->response);
			break;
		case TF_ID:
			manageGetTransformationStatusRequest(&client->response);
			break;
		default:
			/* Can't be reached because of isValidGetId check */
			break;
	}

	client->response.timeTag = timeTags[id];
}

static void manageGetMediaRangeRequest(response_t *response) {
	char *mediaRange = getMediaRangeAsString(getMediaRange(getConfiguration()));
	response->data   = (void *) mediaRange;
	response->dataLength = strlen(mediaRange) + 1;
}

static void manageGetTransformationStatusRequest(response_t *response) {
	uint8_t *transformationState = malloc(sizeof(uint8_t));
	*transformationState		 = getIsTransformationOn(getConfiguration());
	response->data				 = (void *) transformationState;
	response->dataLength		 = sizeof(uint8_t);
}

static void manageGetCommandRequest(response_t *response) {
	char *command = getCommand(getConfiguration());

	response->data		 = (void *) command;
	response->dataLength = strlen(command) + 1;
}

static void manageGetMetricRequest(resId_t id, response_t *response) {
	uint64_t *metric = malloc(sizeof(uint64_t));

	switch (id) {
		case MTR_CN_ID:
			*metric = getConcurrentConections();
			break;
		case MTR_HS_ID:
			*metric = getHistoricAccess();
			break;
		case MTR_BT_ID:
			*metric = getTransferBytes();
			break;
		default:
			break;
	}

	/* Change the 64-bits integer from host-bytes to big-endian */
	*metric				 = htobe64(*metric);
	response->data		 = (void *) metric;
	response->dataLength = sizeof(uint64_t);
}

static uint8_t isValidGetId(resId_t id) {
	return id >= MIME_ID && id <= TF_ID;
}

static uint8_t isValidSetId(resId_t id) {
	return id == MIME_ID || id == CMD_ID || id == TF_ID;
}

static void manageSetRequest(manager_t *client) {
	resId_t id = client->request.id;

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
				if (strcmp(client->request.data, "") != 0) {
					addMediaRange(getMediaRange(getConfiguration()),
								  (char *) client->request.data);
				}
				else {
					resetMediaRangeList(getConfiguration());
				}
				break;
			case CMD_ID:
				setCommand(getConfiguration(), (char *) client->request.data);
				break;
			case TF_ID:
				setTransformationState(getConfiguration(),
									   *((uint8_t *) client->request.data));
				break;
			default:
				/* Can't be reached because of isValidSetId check */
				break;
		}

		client->response.timeTag = timeTags[id];

		return;
	}

	/* Valid ID & Different TIME-TAG */
	client->response.status.generalStatus = ERROR_STATUS;
	client->response.status.timeTagStatus = ERROR_STATUS;
}

static int handleNonAuthenticatedRead(struct selector_key *key) {
	manager_t *client = (manager_t *) key->data;
	char *username;
	char *password;
	uint8_t hasSameVersion;

	int read = recvAuthenticationRequest(key->fd, &username, &password,
										 &hasSameVersion);

	if (read <= 0) {
		errorMessage = getProtocolErrorMessage();
		// TODO: destroy?
		return read;
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
	manager_t *client = (manager_t *) key->data;
	int sent;

	if (client->isAuthenticated && client->authResponseSent) {
		sent = sendResponse(key->fd, client->response);
	}
	else {
		sent = sendAuthenticationResponse(key->fd, client->authResponse);
	}

	if (sent < 0) {
		errorMessage = getProtocolErrorMessage();
		// TODO: destroy?
	}

	if (client->isAuthenticated && !client->authResponseSent) {
		client->authResponseSent = TRUE;
	}

	selector_set_interest(key->s, key->fd, OP_READ);
}

static void handleClose(struct selector_key *key) {
	// TODO: hmmm? what we need to do here?
}

#include <manager.h>
#include <protocol.h>
#include <commandParser.h>
#include <linkedListADT.h>
#include <time.h>
#include <unistd.h>
#include <colors.h>

/* Initialize in zero all time-tags */
timeTag_t timeTags[ID_QUANTITY] = {0};

void *storedData[ID_QUANTITY] = {0};

typedef struct {
	returnCode_t code;
	operation_t operation;
	id_t id;
	void *data;
	uint16_t streamNumber;
} responseToRecv_t;

queueADT_t toRecv				  = NULL;
linkedListADT_t recvFromSetStream = NULL;
linkedListADT_t recvFromGetStream = NULL;

static int authenticate(int server);
static int parseAndSendRequests(int server, uint8_t *byeRead);
static int newCommandHandler(int server, operation_t operation, id_t id,
							 void *data, size_t dataLength, uint8_t *byeRead);
static void invalidCommandHandler();
static int recvAndPrintResponses(int server);
static void setRecvFrom(uint16_t currentStreamNumber,
						linkedListADT_t *recvFromCurrentStream,
						linkedListADT_t *recvFromOtherStream);
static void printInvalidCommand();
static void manageAndPrintResponse(response_t response);
static void manageAndPrintGetResponse(response_t response);
static void manageAndPrintSetResponse(response_t response);
static void printResponseHeader();
static void printDottedSeparator();
static void printContinuousSeparator();

int main(int argc, char const *argv[]) {
	char const *ip = DEFAULT_IP;
	uint16_t port  = DEFAULT_PORT;

	parseIPAndPortFromArguments(&ip, &port, argc, argv);

	int server = establishConnection(ip, port);

	if (server < 0) {
		printf("%s\n", getProtocolErrorMessage());
		printf("Can't establish connection with server\n");
		return -1;
	}

	int authenticationStatus = authenticate(server);

	if (authenticationStatus < 0) {
		printf("Can't authenticate with server\n");
		return -1;
	}

	printContinuousSeparator();
	setPrintStyle(BOLD_GREEN);
	printf("\nWelcome to HTTP Proxy Manager!\n");
	resetPrintStyle();

	uint8_t byeRead;
	int sent;
	int read;

	do {
		printContinuousSeparator();
		setPrintStyle(BOLD);
		printf("\nENTER COMMANDS\n\n");
		resetPrintStyle();

		sent = parseAndSendRequests(server, &byeRead);

		if (sent < 0) {
			printf("%s\n", getProtocolErrorMessage());
			printf("Error while sending requests to server\n");
			return -1;
		}

		read = recvAndPrintResponses(server);

		if (read < 0) {
			printf("%s\n", getProtocolErrorMessage());
			printf("Error while receiving responses from server\n");
			return -1;
		}
	} while (!byeRead);

	close(server);
	// TODO: read all bytes in socket to clean its buffer? close it?

	return 0;
}

static int authenticate(int server) {
	char *username;
	char *password;
	size_t usernameLength;
	size_t passwordLength;
	authenticationResponse_t response;

	printContinuousSeparator();
	setPrintStyle(BOLD);
	printf("\nAUTHENTICATION\n\n");
	resetPrintStyle();

	do {
		username	   = NULL;
		password	   = NULL;
		usernameLength = 0;
		passwordLength = 0;

		parseAuthenticationData(&username, &usernameLength, &password,
								&passwordLength);

		int sent = sendAuthenticationRequest(server, username, usernameLength,
											 password, passwordLength);

		free(username);
		free(password);

		if (sent < 0) {
			printf("Error sending authentication request\n");
			return -1;
		}

		int read = recvAuthenticationResponse(server, &response);

		if (read < 0) {
			printf("Error receiving authentication response\n");
			return -1;
		}

		if (response.status.generalStatus != OK_STATUS) {
			if (response.status.versionStatus != OK_STATUS) {
				printf("Server manages another version of the protocol\n");
				printf("Server version = v%ld\n", response.version);
				return -1;
			}

			if (response.status.authenticationStatus != OK_STATUS) {
				setPrintStyle(RED);
				printf("\nIncorrect username or password. Please, try "
					   "again\n\n\n");
				resetPrintStyle();
			}
		}
	} while (response.status.generalStatus != OK_STATUS);

	return 0;
}

static int parseAndSendRequests(int server, uint8_t *byeRead) {
	operation_t operation;
	id_t id;
	void *data;
	size_t dataLength;
	int sent;
	*byeRead = 0;

	returnCode_t returnCode;

	do {
		data	   = NULL;
		dataLength = 0;
		sent	   = 0;
		returnCode = parseCommand(&operation, &id, &data, &dataLength);

		if (!*byeRead) {
			switch (returnCode) {
				case NEW:
					sent = newCommandHandler(server, operation, id, data,
											 dataLength, byeRead);
					break;
				case INVALID:
					invalidCommandHandler();
					break;
				default:
					/* Nothing to do here */
					break;
			}
		}
	} while (returnCode != SEND && sent >= 0);

	return sent;
}

static int newCommandHandler(int server, operation_t operation, id_t id,
							 void *data, size_t dataLength, uint8_t *byeRead) {
	uint16_t streamNumber;
	int sent;
	*byeRead = 0;

	switch (operation) {
		case BYE_OP:
			*byeRead	 = 1;
			streamNumber = BYE_STREAM;
			sent		 = sendByeRequest(server);
			break;
		case GET_OP:
			streamNumber = GET_STREAM;
			sent		 = sendGetRequest(server, id, timeTags[id]);
			break;
		case SET_OP:
			streamNumber = SET_STREAM;
			sent = sendSetRequest(server, id, timeTags[id], data, dataLength);
			break;
	}

	if (!*byeRead) {
		responseToRecv_t requestSent = {.code		  = NEW,
										.operation	= operation,
										.id			  = id,
										.data		  = data,
										.streamNumber = streamNumber};
		enqueue(&toRecv, &requestSent, sizeof(requestSent));
	}

	return sent;
}

static void invalidCommandHandler() {
	responseToRecv_t invalidCommand = {
		.code		  = INVALID,
		.operation	= 0, /* Could be any operation, will be ignored */
		.id			  = 0, /* Could be any id, will be ignored */
		.streamNumber = 0  /* Could be any stream number, will be ignored */
	};

	enqueue(&toRecv, &invalidCommand, sizeof(invalidCommand));
}

static int recvAndPrintResponses(int server) {
	int totalRead = 0;
	int read;
	response_t *responsePtr = NULL;
	response_t response;
	/* The next response to receive, to print the command responses in order */
	responseToRecv_t *nextToRecv = NULL;
	/* The nextToRecv.streamNumber stream */
	linkedListADT_t recvFromCurrentStream = NULL;
	/* The stream that not match the nextToRecv.streamNumber stream number */
	linkedListADT_t recvFromOtherStream = NULL;

	while (!isEmpty(&toRecv)) {
		nextToRecv = (responseToRecv_t *) getFirst(&toRecv);

		if (nextToRecv->code == INVALID) {
			printInvalidCommand();
		}
		else {
			/* nextToRecv.code is NEW */
			setRecvFrom(nextToRecv->streamNumber, &recvFromCurrentStream,
						&recvFromOtherStream);

			if (isEmpty(&recvFromCurrentStream)) {
				do {
					read = recvResponse(server, &response);

					if (read < 0) {
						return read;
					}

					totalRead += read;

					if (response.streamNumber != nextToRecv->streamNumber) {
						enqueue(&recvFromOtherStream, &response,
								sizeof(response_t));
					}
				} while (response.streamNumber != nextToRecv->streamNumber);
			}
			else {
				responsePtr = getFirst(&recvFromCurrentStream);
				response	= *responsePtr;
			}

			response.operation = nextToRecv->operation;
			response.id		   = nextToRecv->id;

			if (response.operation == SET_OP) {
				response.data = nextToRecv->data;
			}

			manageAndPrintResponse(response);
		}

		free(nextToRecv);
		free(responsePtr);
	}

	return totalRead;
}

static void setRecvFrom(uint16_t currentStreamNumber,
						linkedListADT_t *recvFromCurrentStream,
						linkedListADT_t *recvFromOtherStream) {
	if (currentStreamNumber == GET_STREAM) {
		*recvFromCurrentStream = recvFromGetStream;
		*recvFromOtherStream   = recvFromSetStream;
	}
	else {
		*recvFromCurrentStream = recvFromSetStream;
		*recvFromOtherStream   = recvFromGetStream;
	}
}

static void printInvalidCommand() {
	printResponseHeader();

	setPrintStyle(RED);
	printf("Invalid command\n");
	resetPrintStyle();
}

static void printResponseHeader() {
	printDottedSeparator();
	setPrintStyle(BOLD);
	printf("\nRESPONSE\n\n");
	resetPrintStyle();
}

static void printDottedSeparator() {
	printf("---------------------------------\n");
}

static void printContinuousSeparator() {
	printf("_________________________________\n");
}

static void manageAndPrintResponse(response_t response) {
	printResponseHeader();

	if (response.status.generalStatus == ERROR_STATUS) {
		if (response.status.operationStatus == ERROR_STATUS) {
			printf("Invalid operation\n");
			return;
		}

		if (response.status.idStatus == ERROR_STATUS) {
			printf("Invalid id\n");
			return;
		}
	}

	switch (response.operation) {
		case GET_OP:
			manageAndPrintGetResponse(response);
			break;
		case SET_OP:
			manageAndPrintSetResponse(response);
			break;
		default:
			break;
	}
}

static void manageAndPrintGetResponse(response_t response) {
	if (response.status.timeTagStatus == OK_STATUS) {
		printf("[You already had the last version]\n");
	}
	else {
		printf("[New version gotten]\n");

		if (timeTags[response.id] == 0) {
			printf("[Resource gotten for first time]\n");
		}
		else {
			printf("You previous 'Last modified date' was: %s\n",
				   ctime((const time_t *) &timeTags[response.id]));
		}

		// printf("[manager/main.c][manageAndPrintGetResponse] response.data =
		// "); // TODO LOGGER for (int i = 0; i < response.dataLength; i++) {
		// 	printf(" 0x%02X ", ((uint8_t *) response.data)[i]);
		// }
		// printf("\n\n");

		if (storedData[response.id] != NULL) {
			free(storedData[response.id]);
		}

		storedData[response.id] = response.data;
		timeTags[response.id]   = response.timeTag;
	}

	printf("Last modified at: %s\n",
		   ctime((const time_t *) &timeTags[response.id]));

	/* Print values */
	switch (response.id) {
		case MIME_ID:
			printf("Media range = %s\n\n", (char *) storedData[response.id]);
			break;
		case CMD_ID:
			printf("Command = %s\n\n", (char *) storedData[response.id]);
			break;
		case MTR_CN_ID:
			printf("Concurrent connections = %ld\n\n",
				   *((uint64_t *) storedData[response.id]));
			break;
		case MTR_HS_ID:
			printf("Historic connections = %ld\n\n",
				   *((uint64_t *) storedData[response.id]));
			break;
		case MTR_BT_ID:
			printf("Bytes transfered = %ld\n\n",
				   *((uint64_t *) storedData[response.id]));
			break;
		case TF_ID:
			printf("Transformations state = %s\n\n",
				   *((uint8_t *) storedData[response.id]) ? "on" : "off");
			break;
		default:
			break;
	}
}

static void manageAndPrintSetResponse(response_t response) {
	if (response.status.timeTagStatus == OK_STATUS) {
		setPrintStyle(GREEN);
		printf("Operation successfully performed. You overrided the "
			   "resource!\n\n");
		resetPrintStyle();

		timeTags[response.id] = response.timeTag;

		if (response.id == MIME_ID && strcmp(response.data, "") != 0 &&
			strcmp(storedData[MIME_ID], "") != 0) {
			int storedDataLength = strlen(storedData[MIME_ID]);
			int dataLength		 = strlen(response.data);

			/* + 3 beacuse of: comma, space and null terminated */
			char *data = malloc(storedDataLength + dataLength + 3);

			memcpy(data, storedData[MIME_ID], storedDataLength);
			data[storedDataLength] = ',';
			storedDataLength++;
			data[storedDataLength] = ' ';
			storedDataLength++;
			memcpy(data + storedDataLength, response.data, dataLength);
			data[storedDataLength + dataLength] = '\0';

			free(response.data);

			response.data = data;
		}

		if (storedData[response.id] != NULL) {
			free(storedData[response.id]);
		}

		storedData[response.id] = response.data;
	}
	else {
		setPrintStyle(YELLOW);
		printf("You aren't up to date, you need to get the resource first\n\n");
		resetPrintStyle();
	}
}

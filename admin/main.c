#include <admin.h>
#include <protocol.h>
#include <commandParser.h>
#include <linkedListADT.h>

/* Initialize in zero all time-tags */
timeTag_t timeTags[ID_QUANTITY] = {0};

typedef struct {
	returnCode_t code;
	uint16_t streamNumber;
} responseToRecv_t;

queueADT_t toRecv;
linkedListADT_t recvFromSetStream;
linkedListADT_t recvFromGetStream;

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

/*****************************************************************************\
\*****************************************************************************/

int main(int argc, char const *argv[]) {
	char const *ip = DEFAULT_IP;
	uint16_t port  = DEFAULT_PORT;

	parseIPAndPortFromArguments(&ip, &port, argc, argv);

	int server = establishConnection(ip, port, STREAM_QUANTITY);

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

	uint8_t byeRead;
	int sent;
	int read;

	do {
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

	// TODO: read all bytes in socket to clean its buffer? close it?

	return 0;
}

static int authenticate(int server) {
	char *username;
	char *password;
	size_t usernameLength;
	size_t passwordLength;
	authenticationResponse_t response;

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

		if (response.status.generalStatus != OK) {
			if (response.status.versionStatus != OK) {
				printf("Server manages another version of the protocol\n");
				printf("Server version = v%ld\n", response.version);
				return -1;
			}
			else if (response.status.authenticationStatus != OK) {
				printf("Incorrect username or password. Please, try again\n");
			}
		}
	} while (response.status.generalStatus != OK);

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

		if (data != NULL) {
			free(data);
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
			sent		 = sendByeRequest(server, streamNumber);
			break;
		case GET_OP:
			streamNumber = GET_STREAM;
			sent = sendGetRequest(server, id, timeTags[id], streamNumber);
			break;
		case SET_OP:
			streamNumber = SET_STREAM;
			sent = sendSetRequest(server, id, timeTags[id], data, dataLength,
								  streamNumber);
			break;
	}

	if (!byeRead) {
		responseToRecv_t requestSent = {.code		  = NEW,
										.streamNumber = streamNumber};
		enqueue(&toRecv, &requestSent, sizeof(requestSent));
	}

	return sent;
}

static void invalidCommandHandler() {
	responseToRecv_t invalidCommand = {
		.code		  = INVALID,
		.streamNumber = 0 /* Could be any stream number, will be ignored */
	};

	enqueue(&toRecv, &invalidCommand, sizeof(invalidCommand));
}

static int recvAndPrintResponses(int server) {
	int totalRead = 0;
	int read;
	response_t response;
	/* The next response to receive, to print the command responses in order */
	responseToRecv_t nextToRecv;
	/* The nextToRecv.streamNumber stream */
	linkedListADT_t recvFromCurrentStream;
	/* The stream that not match the nextToRecv.streamNumber stream number */
	linkedListADT_t recvFromOtherStream;

	while (!isEmpty(&toRecv)) {
		nextToRecv = *((responseToRecv_t *) getFirst(&toRecv));

		if (nextToRecv.code == INVALID) {
			printInvalidCommand();
		}
		else {
			/* nextToRecv.code is NEW */
			setRecvFrom(nextToRecv.streamNumber, &recvFromCurrentStream,
						&recvFromOtherStream);

			if (isEmpty(&recvFromCurrentStream)) {
				do {
					read = recvResponse(server, &response);

					if (read < 0) {
						return read;
					}

					totalRead += read;

					if (response.streamNumber != nextToRecv.streamNumber) {
						enqueue(&recvFromOtherStream, &response,
								sizeof(response_t));
					}
				} while (response.streamNumber != nextToRecv.streamNumber);
			}
			else {
				response = *((response_t *) getFirst(&recvFromCurrentStream));
			}

			manageAndPrintResponse(response);
		}
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
	printf("Invalid command\n");
}

static void manageAndPrintResponse(response_t response) {
	// TODO: manage this...
	printf("New response OPCODE = %x\n", response.operation);
}

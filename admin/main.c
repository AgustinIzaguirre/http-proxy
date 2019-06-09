#include <admin.h>
#include <protocol.h>
#include <commandParser.h>
#include <linkedListADT.h>

/* Initialize in zero all time-tags */
timeTag_t timeTags[ID_QUANTITY] = {0};

typedef struct {
	returnCode_t code;
	uint16_t streamNumber;
} request_t;

queueADT_t requests;

linkedListADT_t responsesRecvOnSetStream;
linkedListADT_t responsesRecvOnGetStream;

static void authenticate(int server);

static uint8_t parseAndSendRequests();

static uint8_t newCommandHandler(operation_t operation, id_t id, void *data,
								 size_t dataLength);

static void invalidCommandHandler();

static void recvAndPrintResponses();

int main(int argc, char const *argv[]) {
	char const *ip = DEFAULT_IP;
	uint16_t port  = DEFAULT_PORT; // TODO: is this an int?

	parseIPAndPortFromArguments(&ip, &port, argc, argv);

	int server = establishConnection(ip, port, STREAM_QUANTITY);

	if (server < 0) {
		printf("%s\n", getProtocolErrorMessage());
		printf("Error: Can't establish connection with server\n");
		return -1;
	}

	authenticate(server);

	uint8_t byeRead = 0;

	do {
		byeRead = parseAndSendRequests();
		recvAndPrintResponses();
	} while (!byeRead);

	// TODO: read all bytes in socket to clean its buffer? close it?

	return 0;
}

// TODO: Maybe return 0 or -1 indicating if version error, to leave admin
static void authenticate(int server) {
	char *username;
	char *password;
	size_t usernameLength;
	size_t passwordLength;
	uint8_t response;

	do {
		username	   = NULL;
		password	   = NULL;
		usernameLength = 0;
		passwordLength = 0;

		parseAuthenticationData(&username, &usernameLength, &password,
								&passwordLength);

		sendAuthenticationRequest(server, username, usernameLength, password,
								  passwordLength);

		free(username);
		free(password);

		response = recvAuthenticationResponse(server);

		// TODO: manage version error to show server current version
		if ((response & GENERAL_STATUS_MASK) == GENERAL_STATUS_MASK) {
			if ((response & VERSION_STATUS_MASK) == VERSION_STATUS_MASK) {
				// return VERSION_STATUS_MASK;
			}
			else if ((response & AUTH_STATUS_MASK) == AUTH_STATUS_MASK) {
				printf("Incorrect username or password. Try again\n");
			}
		}
	} while ((response & GENERAL_STATUS_MASK) == GENERAL_STATUS_MASK);
}

/* Return different than zero if a BYE command was read */
static uint8_t parseAndSendRequests() {
	operation_t operation;
	id_t id;
	void *data;
	size_t dataLength;

	uint8_t byeRead = 0;

	returnCode_t returnCode;

	do {
		data	   = NULL;
		dataLength = 0;
		returnCode = parseCommand(&operation, &id, &data, &dataLength);

		if (!byeRead) {
			switch (returnCode) {
				case NEW:
					byeRead =
						newCommandHandler(operation, id, data, dataLength);
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
	} while (returnCode != SEND);

	return byeRead;
}

static uint8_t newCommandHandler(operation_t operation, id_t id, void *data,
								 size_t dataLength) {
	uint8_t byeRead = 0;
	uint16_t streamNumber;

	switch (operation) {
		case BYE_OP:
			byeRead		 = 1;
			streamNumber = BYE_STREAM;
			sendByeRequest(streamNumber);
			break;
		case GET_OP:
			streamNumber = GET_STREAM;
			sendGetRequest(id, timeTags[id], streamNumber);
			break;
		case SET_OP:
			streamNumber = SET_STREAM;
			sendPostRequest(id, timeTags[id], data, dataLength, streamNumber);
			break;
	}

	request_t requestSent = {.code = NEW, .streamNumber = streamNumber};

	// TODO: if (byeRead) { add to the queue...or not? }
	enqueue(&requests, &requestSent, sizeof(requestSent));

	return byeRead;
}

static void invalidCommandHandler() {
	request_t invalidCommand = {
		.code		  = INVALID,
		.streamNumber = 0 /* Could be any stream number, will be ignored */
	};

	enqueue(&requests, &invalidCommand, sizeof(invalidCommand));
}

static void recvAndPrintResponses() {
	/* code */
}

#include <admin.h>
#include <protocol.h>
#include <commandParser.h>

/* Initialize in zero all time-tags */
timeTag_t timeTags[ID_QUANTITY] = {0};

typedef struct {
	returnCode_t code;
	uint16_t streamNumber;
} requestParsed_t; // TODO: check this name to be semanthically better

queueADT_t requestsParsed;

linkedListADT_t responsesRecvOnSetStream;
linkedListADT_t responsesRecvOnGetStream;

int main(int argc, char const *argv[]) {
	// TODO: receive from argv IP and PORT for the server to administrate
	int server = establishConnection(DEFAULT_IP, DEFAULT_PORT, STREAM_QUANTITY);

	if (server < 0) {
		printf("Error: Can't establish connection with server\n");
		return -1;
	}

	autenticate(server);

	uint8_t byeRead = 0;

	do {
		byeRead = parseAndSendRequests();
		recvAndPrintResponses();
	} while (!byeRead);

	// TODO: read all bytes in socket to clean its buffer? close it?

	return 0;
}

// TODO: Maybe return 0 or -1 indicating if version error, to leave admin
void autenticate(int server) {
	char *username;
	char *password;
	size_t usernameLength;
	size_t passwordLength;

	do {
		*username	  = NULL;
		*password	  = NULL;
		usernameLength = 0;
		passwordLength = 0;

		parseAuthenticationData(&username, &usernameLength, &password,
								&passwordLength);

		sendAuthenticationRequest(server, &username, &usernameLength, &password,
								  &passwordLength);

		free(username);
		free(password);

		uint8_t response = recvAuthenticationResponse(server);

		// TODO: manage version error to show server current version
		if ((response & GENERAL_ERROR) == GENERAL_ERROR) {
			if ((response & VERSION_ERROR) == VERSION_ERROR) {
				return VERSION_ERROR;
			}
			else if ((response & SIGNIN_ERROR) == SIGNIN_ERROR) {
				printf("Incorrect username or password. Try again\n");
			}
		}
	} while ((response & GENERAL_ERROR) == GENERAL_ERROR);
}

/* Return different than zero if a BYE command was read */
uint8_t parseAndSendRequests() {
	operation_t operation;
	id_t id;
	void *data;
	size_t dataLength;

	uint8_t byeRead = 0;

	returnCode_t returnCode;

	do {
		*data	  = NULL;
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

uint8_t newCommandHandler(operation_t operation, id_t id, void *data,
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
			sendGetRequest(timeTags[id], streamNumber);
			break;
		case SET_OP:
			streamNumber = SET_STREAM;
			sendPostRequest(timeTags[id], data, dataLength, streamNumber);
			break;
	}

	// TODO: if (byeRead) add to the queue...or not?
	// TODO: add (NEW, streamNumber) pair to the queue

	return byeRead;
}

void invalidCommandHandler() {
	// TODO: add (INVALID, whateverStreamNumber) pair to the queue
}

void recvAndPrintResponses() {
}

#include <admin.h>
#include <protocol.h>
#include <commandParser.h>

int main(int argc, char const *argv[]) {
	/* TODO: receive from argv IP and PORT for the server to administrate */
	int server = establishConnection(DEFAULT_IP, DEFAULT_PORT);

	if (server < 0) {
		printf("Error: Can't establish connection with server\n");
		return -1;
	}

	autenticate(server);

	do {
		uint8_t byeRead = parseAndSendRequests();
		recvAndPrintResponses();
	} while (!byeRead);

	return 0;
}

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

		uint8_t response = readAuthenticationResponse(server);

		/* TODO: manage version error to show server current version */
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

void parseAndSendRequests() {
	operation_t operation;
	id_t id;
	void *data;
	size_t dataLength;
	returnCode_t returnCode;

	do {
		*data	  = NULL;
		dataLength = 0;
		returnCode = parseCommand(&operation, &id, &data, &dataLength);

		switch (returnCode) {
			case NEW:
				// TODO: manage new command
				break;
			case INVALID:
				// TODO: manage invalid command
				break;
		}

		free(data);
	} while (returnCode != SEND);
}

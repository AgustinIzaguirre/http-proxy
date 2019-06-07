#include <protocol.h>
#include <netinet/sctp.h>

static void sendSCTPMsg(int server, void *msg, size_t msgLength,
						int streamNumber) {
	sctp_sendmsg(server, msg, msgLength, NULL, 0, 0, 0, streamNumber, 0, 0);
}

int establishConnection(char *serverIP, unsigned int serverPort) {
	return prepareSCTPSocket(serverIP, serverPort);
}

static int prepareSCTPSocket(char *serverIP, unsigned int serverPort) {
	struct sockaddr_in serverAddress;
	struct sctp_initmsg initMsg;
	struct sctp_event_subscribe events;

	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	if (server < 0) {
		printf("Error: Can not create SCTP Socket\n");
		return -1;
	}

	memset(&initMsg, 0, sizeof(initMsg));

	initMsg.sinit_num_ostreams  = NUM_STREAMS;
	initMsg.sinit_max_instreams = NUM_STREAMS;
	initMsg.sinit_max_attempts  = MAX_ATTEMPTS;

	setsockopt(server, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg));

	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port   = htons(serverPort);

	/* Convert IPv4 and IPv6 addresses from text to binary form */
	int convert = inet_pton(AF_INET, serverIP, &serverAddress.sin_addr.s_addr);

	if (convert <= 0) {
		printf("Error: Invalid IP address\n");
		return -1;
	}

	int connection = connect(server, (struct sockaddr *) &serverAddress,
							 sizeof(serverAddress));

	if (connection < 0) {
		printf("errno = %d, error_message = %s\n", errno, strerror(errno));
		printf("Error: Connection failed\n");
		return -1;
	}

	memset((void *) &events, 0, sizeof(events));

	/*	By default returns only the data read.
		To know which stream the data came we enable the data_io_event
		The info will be in sinfo_stream in sctp_sndrcvinfo struct */
	events.sctp_data_io_event = 1;
	setsockopt(server, SOL_SCTP, SCTP_EVENTS, (const void *) &events,
			   sizeof(events));

	return server;
}

void sendAuthenticationRequest(int server, char *username,
							   size_t usernameLength, char *password,
							   size_t passwordLength) {
	/* Build authentication request */
	/* + 2 because username and password are null terminated */
	size_t length = VERSION_BYTES + usernameLength + passwordLength + 2;
	uint8_t *data = calloc(length, sizeof(*data));

	setVersionBytes((void *) data);

	for (int i = 0; i < usernameLength; i++) {
		data[VERSION_BYTES + i] = username[i];
	}

	data[VERSION_BYTES + usernameLength] = 0;

	for (int i = 0; i < passwordLength; i++) {
		data[VERSION_BYTES + usernameLength + 1 + i] = password[i];
	}

	data[VERSION_BYTES + usernameLength + 1 + passwordLength] = 0;

	sendSCTPMsg(server, (void *) data, length, AUTHENTICATION_STREAM);

	free(data);
}

void setVersionBytes(void *data) {
	((uint8_t *) data)[0] = VERSION_BYTE;
}

/* If maxLengthToRead is not set, reads all the bytes in the socket */
static size_t receiveSCTPMsg(int server, void **buffer, size_t maxLengthToRead,
							 struct sctp_sndrcvinfo *sndRcvInfo, int *flags) {
	size_t bytesRead = 0;

	if (maxLengthToRead > 0) {
		*((uint8_t **) buffer) = calloc(maxLengthToRead, sizeof(uint8_t));
		bytesRead =
			sctp_recvmsg(server, *buffer, maxLengthToRead,
						 (struct sockaddr *) NULL, 0, sndRcvInfo, flags);
	}
	else {
		size_t read			   = 0;
		*((uint8_t **) buffer) = NULL;

		do {
			*((uint8_t **) buffer) =
				realloc(*((uint8_t **) buffer),
						(bytesRead + RCV_BLOCK) * sizeof(uint8_t));

			read = sctp_recvmsg(server, (*(uint8_t **) buffer) + bytesRead,
								RCV_BLOCK, (struct sockaddr *) NULL, 0,
								sndRcvInfo, flags);

			bytesRead += read;
		} while (read == RCV_BLOCK);
	}

	return bytesRead;
}

/* TODO: Manage version number if gets errorVersion */
uint8_t readAuthenticationResponse(int server) {
	uint8_t responseByte;
	uint8_t *response;
	struct sctp_sndrcvinfo sndRcvInfo;
	int flags = 0;

	receiveSCTPMsg(server, (void **) &response, 0, &sndRcvInfo, &flags);

	responseByte = *response;
	free(response);

	return responseByte;
}

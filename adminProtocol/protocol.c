#include <protocol.h>

static char *errorMessage = NULL;

static int sendSCTPMsg(int server, void *msg, size_t msgLength,
					   uint16_t streamNumber);

static int prepareSCTPSocket(const char *serverIP, uint16_t serverPort,
							 uint16_t streamQuantity);

static void setVersionBytes(void *data);

/* If maxLengthToRead is not set, reads all the bytes in the socket */
static int receiveSCTPMsg(int server, void **buffer, int maxLengthToRead,
						  struct sctp_sndrcvinfo *sndRcvInfo, int *flags);

static void loadRecvAuthenticationResponse(
	uint8_t *response, authenticationResponse_t *authenticationResponse);

static uint64_t getVersion(uint8_t *response);

/*****************************************************************************\
\*****************************************************************************/

static int sendSCTPMsg(int server, void *msg, size_t msgLength,
					   uint16_t streamNumber) {
	return sctp_sendmsg(server, msg, msgLength, NULL, 0, 0, 0, streamNumber, 0,
						0);
}

int establishConnection(const char *serverIP, uint16_t serverPort,
						uint16_t streamQuantity) {
	return prepareSCTPSocket(serverIP, serverPort, streamQuantity);
}

static int prepareSCTPSocket(const char *serverIP, uint16_t serverPort,
							 uint16_t streamQuantity) {
	struct sockaddr_in serverAddress;
	struct sctp_initmsg initMsg;
	struct sctp_event_subscribe events;

	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	CHECK_FOR_ERROR(server);

	memset(&initMsg, 0, sizeof(initMsg));

	initMsg.sinit_num_ostreams  = streamQuantity;
	initMsg.sinit_max_instreams = streamQuantity;
	initMsg.sinit_max_attempts  = MAX_ATTEMPTS;

	setsockopt(server, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg));

	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port   = htons(serverPort);

	/* Convert IPv4 and IPv6 addresses from text to binary form */
	int convert = inet_pton(AF_INET, serverIP, &serverAddress.sin_addr.s_addr);

	CHECK_FOR_ERROR(convert);

	int connection = connect(server, (struct sockaddr *) &serverAddress,
							 sizeof(serverAddress));

	CHECK_FOR_ERROR(connection);

	memset((void *) &events, 0, sizeof(events));

	/*	By default returns only the data read.
		To know which stream the data came we enable the data_io_event
		The info will be in sinfo_stream in sctp_sndrcvinfo struct */
	events.sctp_data_io_event = 1;
	setsockopt(server, SOL_SCTP, SCTP_EVENTS, (const void *) &events,
			   sizeof(events));

	return server;
}

static void setVersionBytes(void *data) {
	((uint8_t *) data)[0] = VERSION_BYTE;
}

int sendAuthenticationRequest(int server, char *username, size_t usernameLength,
							  char *password, size_t passwordLength) {
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

	int sent =
		sendSCTPMsg(server, (void *) data, length, AUTHENTICATION_STREAM);

	free(data);

	CHECK_FOR_ERROR(sent);

	return sent;
}

/* If maxLengthToRead is not set, reads all the bytes in the socket */
static int receiveSCTPMsg(int server, void **buffer, int maxLengthToRead,
						  struct sctp_sndrcvinfo *sndRcvInfo, int *flags) {
	int bytesRead = 0;

	if (maxLengthToRead > 0) {
		*((uint8_t **) buffer) = calloc(maxLengthToRead, sizeof(uint8_t));
		bytesRead =
			sctp_recvmsg(server, *buffer, maxLengthToRead,
						 (struct sockaddr *) NULL, 0, sndRcvInfo, flags);

		CHECK_FOR_ERROR(bytesRead);
	}
	else {
		int read			   = 0;
		*((uint8_t **) buffer) = NULL;

		do {
			*((uint8_t **) buffer) =
				realloc(*((uint8_t **) buffer),
						(bytesRead + RECV_BYTES) * sizeof(uint8_t));

			read = sctp_recvmsg(server, (*(uint8_t **) buffer) + bytesRead,
								RECV_BYTES, (struct sockaddr *) NULL, 0,
								sndRcvInfo, flags);

			CHECK_FOR_ERROR(read);

			bytesRead += read;
		} while (read == RECV_BYTES);
	}

	return bytesRead;
}

void *formatData(void *data, size_t dataLength, size_t *formattedDataLength) {
	if (dataLength <= 0) {
		*formattedDataLength = 0;
		return NULL;
	}

	int paddingBytesNeeded = 0;
	int dataBlocksNeeded   = dataLength / CONCRET_DATA_BLOCK_BYTES;

	if (dataLength % CONCRET_DATA_BLOCK_BYTES != 0) {
		paddingBytesNeeded =
			CONCRET_DATA_BLOCK_BYTES - dataLength % CONCRET_DATA_BLOCK_BYTES;
		dataBlocksNeeded++;
	}

	*formattedDataLength = dataBlocksNeeded * (DATA_BLOCK_BYTES);
	int dataIndex		 = 0;

	uint8_t *formattedData = calloc(*formattedDataLength, sizeof(uint8_t));
	int byteOffsetInBlock;
	int bytesToCopy;

	for (int blockNumber = 0; blockNumber < dataBlocksNeeded; blockNumber++) {
		byteOffsetInBlock = 0;

		formattedData[blockNumber * DATA_BLOCK_BYTES + byteOffsetInBlock] =
			blockNumber == dataBlocksNeeded - 1 ? IS_FINAL_BYTE :
												  NOT_FINAL_BYTE;
		byteOffsetInBlock++;

		bytesToCopy = CONCRET_DATA_BLOCK_BYTES;

		if (blockNumber == 0) {
			byteOffsetInBlock += paddingBytesNeeded;
			bytesToCopy -= paddingBytesNeeded;
		}

		formattedData[blockNumber * DATA_BLOCK_BYTES + byteOffsetInBlock] =
			START_DATA_BYTE;
		byteOffsetInBlock++;

		memcpy(formattedData + blockNumber * DATA_BLOCK_BYTES +
				   byteOffsetInBlock,
			   (uint8_t *) data + dataIndex, bytesToCopy);
		dataIndex += bytesToCopy;
	}

	return (void *) formattedData;
}

int recvAuthenticationResponse(
	int server, authenticationResponse_t *authenticationResponse) {
	uint8_t *response;
	struct sctp_sndrcvinfo sndRcvInfo;
	int flags = 0;

	int read =
		receiveSCTPMsg(server, (void **) &response, 0, &sndRcvInfo, &flags);

	CHECK_FOR_ERROR(read);

	loadRecvAuthenticationResponse(response, authenticationResponse);

	free(response);

	return read;
}

static void loadRecvAuthenticationResponse(
	uint8_t *response, authenticationResponse_t *authenticationResponse) {
	uint8_t firstByte;
	firstByte = *response;

	if (firstByte & GENERAL_STATUS_MASK) {
		authenticationResponse->status.generalStatus = ERROR;

		if (firstByte & VERSION_STATUS_MASK) {
			authenticationResponse->status.versionStatus = ERROR;
			authenticationResponse->version				 = getVersion(response);
		}
		if (firstByte & AUTH_STATUS_MASK) {
			authenticationResponse->status.authenticationStatus = ERROR;
		}
	}
	else {
		authenticationResponse->status.generalStatus		= OK;
		authenticationResponse->status.versionStatus		= OK;
		authenticationResponse->status.authenticationStatus = OK;
		authenticationResponse->version						= VERSION;
	}
}

static uint64_t getVersion(uint8_t *response) {
	uint64_t version				 = 0;
	int bitQuantityForRepresentation = 0;
	int byteIndex					 = 0;
	uint8_t byte					 = response[byteIndex];
	byte							 = byte << 3; /* Avoid status bits */

	while (byte & 0x80) {
		bitQuantityForRepresentation++;
		byte = byte << 1;

		if (bitQuantityForRepresentation % 8 == 0) {
			byteIndex++;
			byte = response[byteIndex];
		}
	}

	while (bitQuantityForRepresentation > 0) {
		bitQuantityForRepresentation--;
		version = version << 1;
		version = version | (byte & 0x80 ? 0x01 : 0x00);
		byte	= byte << 1;
	}

	return version;
}

char *getProtocolErrorMessage() {
	if (errorMessage != NULL) {
		return errorMessage;
	}

	return "Unknown protocol error";
}

int sendByeRequest(uint16_t streamNumber) {
	/* code */
	return 0;
}

int sendGetRequest(uint8_t id, timeTag_t timeTag, uint16_t streamNumber) {
	/* code */
	return 0;
}

int sendPostRequest(uint8_t id, timeTag_t timeTag, void *data,
					size_t dataLength, uint16_t streamNumber) {
	/* code */
	return 0;
}

int recvResponse(response_t *response) {
	/* code */
	return 0;
}

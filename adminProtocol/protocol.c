#include <protocol.h>

static char *errorMessage = NULL;

static int sendSCTPMsg(int fd, void *msg, size_t msgLength,
					   uint16_t streamNumber);

static int prepareSCTPSocket(const char *serverIP, uint16_t serverPort,
							 uint16_t streamQuantity);

static void setVersionBytes(void *data);

/* If maxLengthToRead is not set, reads all the bytes in the socket */
static int receiveSCTPMsg(int fd, void **buffer, int maxLengthToRead,
						  struct sctp_sndrcvinfo *sndRcvInfo, int *flags);

static void loadRecvAuthenticationResponse(
	uint8_t *response, authenticationResponse_t *authenticationResponse);

static uint64_t getVersion(uint8_t *response);

static void *formatData(void *data, size_t dataLength,
						size_t *formattedDataLength);

static int getConcretData(int fd, uint8_t **data, size_t *dataLength);

static int recvGetResponse(int server, response_t *response);

static int recvSetResponse(int server, response_t *response);

static int recvTimeTag(int fd, timeTag_t *timeTag);

static int recvHeadByte(int fd, uint8_t *headByte, uint16_t *streamNumber);

static int recvHeadByteAndLoadResponseInfo(int fd, response_t *response);

/*****************************************************************************\
\*****************************************************************************/

static int sendSCTPMsg(int fd, void *msg, size_t msgLength,
					   uint16_t streamNumber) {
	return sctp_sendmsg(fd, msg, msgLength, NULL, 0, 0, 0, streamNumber, 0, 0);
}

/* If maxLengthToRead is not set, reads all the bytes in the socket fd */
static int receiveSCTPMsg(int fd, void **buffer, int maxLengthToRead,
						  struct sctp_sndrcvinfo *sndRcvInfo, int *flags) {
	int bytesRead = 0;

	if (maxLengthToRead > 0) {
		*((uint8_t **) buffer) = calloc(maxLengthToRead, sizeof(uint8_t));
		bytesRead =
			sctp_recvmsg(fd, *buffer, maxLengthToRead, (struct sockaddr *) NULL,
						 0, sndRcvInfo, flags);

		CHECK_FOR_ERROR(bytesRead);
	}
	else {
		int read			   = 0;
		*((uint8_t **) buffer) = NULL;

		do {
			*((uint8_t **) buffer) =
				realloc(*((uint8_t **) buffer),
						(bytesRead + RECV_BYTES) * sizeof(uint8_t));

			read =
				sctp_recvmsg(fd, (*(uint8_t **) buffer) + bytesRead, RECV_BYTES,
							 (struct sockaddr *) NULL, 0, sndRcvInfo, flags);

			CHECK_FOR_ERROR(read);

			bytesRead += read;
		} while (read == RECV_BYTES);
	}

	return bytesRead;
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

static void *formatData(void *data, size_t dataLength,
						size_t *formattedDataLength) {
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

static int getConcretData(int fd, uint8_t **data, size_t *dataLength) {
	struct sctp_sndrcvinfo sndRcvInfo;
	uint8_t *buffer		= NULL;
	int flags			= 0;
	*dataLength			= 0;
	*data				= NULL;
	int offset			= 0;
	uint8_t isFinalByte = 0;
	int totalRead		= 0;
	int read;

	do {
		if (buffer != NULL) {
			free(buffer);
			buffer = NULL;
		}

		read = receiveSCTPMsg(fd, (void **) &buffer, DATA_BLOCK_BYTES,
							  &sndRcvInfo, &flags);

		if (read < 0) {
			return read;
		}

		totalRead += read;

		*data = realloc(*data, *dataLength + CONCRET_DATA_BLOCK_BYTES);

		offset		= 0;
		isFinalByte = (*data)[offset];
		offset++;

		while (buffer[offset] != START_DATA_BYTE) {
			offset++; /* Search for the START_DATA_BYTE */
		}

		offset++; /* Now points to START_DATA_BYTE, so we avoid it... */

		while (offset < DATA_BLOCK_BYTES) {
			(*data)[(*dataLength)++] = buffer[offset++];
		}
	} while (isFinalByte != IS_FINAL_BYTE);

	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}

	return totalRead;
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

int sendRequest(int server, request_t request) {
	switch (request.operation) {
		case BYE_OP:
			return sendByeRequest(server, request.streamNumber);
		case GET_OP:
			return sendGetRequest(server, request.id, request.timeTag,
								  request.streamNumber);
		case SET_OP:
			return sendSetRequest(server, request.id, request.timeTag,
								  request.data, request.dataLength,
								  request.streamNumber);
		default:
			errorMessage = "Invalid opcode";
			return -1;
	}
}

int sendByeRequest(int server, uint16_t streamNumber) {
	uint8_t byeRequest = BYE_MASK;

	return sendSCTPMsg(server, (void *) &byeRequest, 1, streamNumber);
}

int sendGetRequest(int server, uint8_t id, timeTag_t timeTag,
				   uint16_t streamNumber) {
	size_t length		= sizeof(uint8_t) + sizeof(timeTag_t);
	uint8_t *getRequest = malloc(length);

	getRequest[0] = GET_MASK | (id & ID_MASK);

	memcpy(getRequest + sizeof(uint8_t), &timeTag, sizeof(timeTag));

	int sent = sendSCTPMsg(server, (void *) getRequest, length, streamNumber);

	free(getRequest);

	return sent;
}

int sendSetRequest(int server, uint8_t id, timeTag_t timeTag, void *data,
				   size_t dataLength, uint16_t streamNumber) {
	size_t formattedDataLength;
	void *formattedData = formatData(data, dataLength, &formattedDataLength);
	size_t length = sizeof(uint8_t) + sizeof(timeTag_t) + formattedDataLength;
	uint8_t *setRequest = malloc(length);

	setRequest[0] = SET_MASK | (id & ID_MASK);

	memcpy(setRequest + sizeof(uint8_t), &timeTag, sizeof(timeTag));
	memcpy(setRequest + sizeof(uint8_t) + sizeof(timeTag_t), formattedData,
		   formattedDataLength);

	int sent = sendSCTPMsg(server, (void *) setRequest, length, streamNumber);

	free(formattedData);
	free(setRequest);

	return sent;
}

int recvResponse(int server, response_t *response) {
	int totalRead = 0;
	int read;

	/* Read first byte and loads information */
	read = recvHeadByteAndLoadResponseInfo(server, response);

	totalRead += read;

	if (read < 0) {
		return read;
	}

	response->data		 = NULL;
	response->dataLength = 0;

	if (response->status.operationStatus == OK &&
		response->status.idStatus == OK) {
		switch (response->operation) {
			case GET_OP:
				read = recvGetResponse(server, response);
				break;
			case SET_OP:
				read = recvSetResponse(server, response);
				break;
			default:
				/* Can't be reached, operationStatus must be ERROR */
				break;
		}
	}

	if (read < 0) {
		return read;
	}

	totalRead += read;

	return totalRead;
}

static int recvGetResponse(int server, response_t *response) {
	/* If timeTagStatus = OK you already had the last version of the resource */
	if (response->status.timeTagStatus == OK) {
		response->data		 = NULL;
		response->dataLength = 0;
		return 0;
	}

	int totalRead = 0;
	int read;

	read = recvTimeTag(server, &response->timeTag);

	if (read < 0) {
		return read;
	}

	totalRead += read;

	read = getConcretData(server, (uint8_t **) &response->data,
						  &response->dataLength);

	if (read < 0) {
		return read;
	}

	totalRead += read;

	return totalRead;
}

static int recvSetResponse(int server, response_t *response) {
	response->data		 = NULL;
	response->dataLength = 0;

	int read = 0;

	/* If timeTagStatus is OK, you could override resource */
	if (response->status.timeTagStatus == OK) {
		read = recvTimeTag(server, &response->timeTag);
	}

	return read;
}

static int recvTimeTag(int fd, timeTag_t *timeTag) {
	struct sctp_sndrcvinfo sndRcvInfo;
	int flags		  = 0;
	timeTag_t *buffer = NULL;

	int read = receiveSCTPMsg(fd, (void **) &buffer, sizeof(timeTag_t),
							  &sndRcvInfo, &flags);

	if (buffer != NULL) {
		*timeTag = *buffer;
		free(buffer);
	}

	return read;
}

/* Read and loads head byte and stream number */
static int recvHeadByte(int fd, uint8_t *headByte, uint16_t *streamNumber) {
	struct sctp_sndrcvinfo sndRcvInfo;
	int flags		= 0;
	uint8_t *buffer = NULL;

	int read = receiveSCTPMsg(fd, (void **) &buffer, 1, &sndRcvInfo, &flags);

	if (buffer != NULL) {
		*headByte = *buffer;
		free(buffer);
	}

	CHECK_FOR_ERROR(read);

	*streamNumber = sndRcvInfo.sinfo_stream;

	return read;
}

/* Read first byte and loads information to response struct */
static int recvHeadByteAndLoadResponseInfo(int fd, response_t *response) {
	uint8_t headByte;
	int read = recvHeadByte(fd, &headByte, &response->streamNumber);

	if (read < 0) {
		return read;
	}

	response->status.generalStatus =
		headByte & GENERAL_STATUS_MASK ? ERROR : OK;
	response->status.operationStatus =
		headByte & OPCODE_STATUS_MASK ? ERROR : OK;
	response->status.idStatus	  = headByte & ID_STATUS_MASK ? ERROR : OK;
	response->status.timeTagStatus = headByte & TTAG_STATUS_MASK ? ERROR : OK;

	switch (headByte & OPCODE_MASK) {
		case GET_MASK:
			response->operation = GET_OP;
			break;
		case SET_MASK:
			response->operation = SET_OP;
			break;
		default:
			/* If reached, operationStatus must be ERROR */
			break;
	}

	response->id = headByte & ID_MASK;

	return read;
}

int recvRequest(int client, request_t *request) {
	uint8_t headByte;
	int read = recvHeadByte(client, &headByte, &request->streamNumber);

	// TODO: implement

	return read;
}

static int recvGetRequest(int client, request_t *request) {
	// TODO: implement
	return 0;
}

static int recvSetRequest(int client, request_t *request) {
	// TODO: implement
	return 0;
}

int recvAuthenticationRequest(int client, char **username, char **password) {
	// TODO: implement
	return 0;
}

int sendAuthenticationResponse(
	int client, authenticationResponse_t authenticationResponse) {
	// TODO: implement
	return 0;
}

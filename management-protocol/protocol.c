#include <protocol.h>

static int sendSCTPMsg(int fd, void *msg, size_t msgLength,
					   uint16_t streamNumber);

/* If maxLengthToRead is not set, reads all the bytes in the socket fd */
static int receiveSCTPMsg(int fd, void **buffer, int maxLengthToRead,
						  struct sctp_sndrcvinfo *sndRcvInfo, int *flags);

static int prepareSCTPSocket(const char *serverIP, uint16_t serverPort);

static void setVersionBytes(void *data);

static void *formatData(void *data, size_t dataLength,
						size_t *formattedDataLength);

static int getConcretData(int fd, uint8_t **data, size_t *dataLength);

static void loadRecvAuthenticationResponse(
	uint8_t *response, authenticationResponse_t *authenticationResponse);

static uint64_t getVersion(uint8_t *response);

static int recvGetResponse(int server, response_t *response);

static int recvSetResponse(int server, response_t *response);

static int recvTimeTag(int fd, timeTag_t *timeTag);

/* Read and loads head byte and stream number */
static int recvHeadByte(int fd, uint8_t *headByte, uint16_t *streamNumber);

/* Read first byte and loads information to response struct */
static int recvHeadByteAndLoadResponseInfo(int fd, response_t *response);

static int recvGetRequest(int client, request_t *request);

static int recvSetRequest(int client, request_t *request);

/* Read first byte and loads information to request struct */
static int recvHeadByteAndLoadRequestInfo(int fd, request_t *request);

static char *allocateAndCopyString(char *source, size_t *length);

static uint8_t getResponseHeadByte(responseStatus_t status);

static int prepareAndBindSCTPSocket(uint16_t port, char *ipFilter);

static char *errorMessage = NULL;
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

int establishConnection(const char *serverIP, uint16_t serverPort) {
	return prepareSCTPSocket(serverIP, serverPort);
}

static int prepareSCTPSocket(const char *serverIP, uint16_t serverPort) {
	struct sockaddr_in serverAddress;
	struct sctp_initmsg initMsg;
	struct sctp_event_subscribe events;

	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	CHECK_FOR_ERROR(server);

	memset(&initMsg, 0, sizeof(initMsg));

	initMsg.sinit_num_ostreams  = STREAM_QUANTITY;
	initMsg.sinit_max_instreams = STREAM_QUANTITY;
	initMsg.sinit_max_attempts  = MAX_ATTEMPTS;

	setsockopt(server, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg));

	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port   = htons(serverPort);

	/* Convert IPv4 and (TODO: IPv6) addresses from text to binary form */
	int convert = inet_pton(AF_INET, serverIP, &serverAddress.sin_addr.s_addr);

	if (convert <= 0) {
		errorMessage = "Invalid Server IP, fails to convert it";
		return -1;
	}

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

int bindAndGetServerSocket(uint16_t port, char *ipFilter) {
	return prepareAndBindSCTPSocket(port, ipFilter);
}

static int prepareAndBindSCTPSocket(uint16_t port, char *ipFilter) {
	struct addrinfo hint;
	struct addrinfo *res = NULL;
	int ret;

	memset(&hint, 0, sizeof(hint));

	hint.ai_family = PF_UNSPEC;
	hint.ai_flags  = AI_NUMERICHOST;

	ret = getaddrinfo(ipFilter, NULL, &hint, &res);

	if (ret != 0) {
		errorMessage = "Can't get interface address info\n";
		return -1;
	}

	struct sockaddr_storage *addr = calloc(1, sizeof(struct sockaddr_storage));
	struct sctp_initmsg initMsg;
	int convert;

	switch (res->ai_family) {
		case AF_INET:
			addr->ss_family							  = AF_INET;
			((struct sockaddr_in *) addr)->sin_family = AF_INET;
			((struct sockaddr_in *) addr)->sin_port   = htons(port);
			convert =
				inet_pton(addr->ss_family, ipFilter,
						  &(((struct sockaddr_in *) addr)->sin_addr.s_addr));
			break;
		case AF_INET6:
			addr->ss_family								= AF_INET6;
			((struct sockaddr_in6 *) addr)->sin6_family = AF_INET6;
			((struct sockaddr_in6 *) addr)->sin6_port   = htons(port);
			convert =
				inet_pton(addr->ss_family, ipFilter,
						  &(((struct sockaddr_in6 *) addr)->sin6_addr.s6_addr));
			break;
	}

	if (convert == 0) {
		errorMessage = "Invalid IP interface, fails to convert it";
		return -1;
	}

	CHECK_FOR_ERROR(convert);

	int fd = socket(addr->ss_family, SOCK_STREAM, IPPROTO_SCTP);

	CHECK_FOR_ERROR(fd);

	/* If server fails doesn't have to wait to reuse address */
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	if (addr->ss_family == AF_INET6) {
		/* If AF_INET6 avoid  */
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){0}, sizeof(int));
	}

	int binded = bind(fd, (struct sockaddr *) addr, sizeof(*addr));

	CHECK_FOR_ERROR(binded);

	memset(&initMsg, 0, sizeof(initMsg));

	initMsg.sinit_num_ostreams  = STREAM_QUANTITY;
	initMsg.sinit_max_instreams = STREAM_QUANTITY;
	initMsg.sinit_max_attempts  = MAX_ATTEMPTS;

	setsockopt(fd, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg));

	struct sctp_event_subscribe events;

	memset((void *) &events, 0, sizeof(events));

	/*	By default returns only the data read.
		To know which stream the data came we enable the data_io_event
		The info will be in sinfo_stream in sctp_sndrcvinfo struct */
	events.sctp_data_io_event = 1;
	setsockopt(fd, SOL_SCTP, SCTP_EVENTS, (const void *) &events,
			   sizeof(events));

	return fd;
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
			CONCRET_DATA_BLOCK_BYTES - (dataLength % CONCRET_DATA_BLOCK_BYTES);
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
			(blockNumber == (dataBlocksNeeded - 1)) ? IS_FINAL_BYTE :
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

	// printf("[protocol.c][formatData] FORMATTED-DATA = "); // TODO LOGGER
	// for (int i = 0; i < *formattedDataLength; i++) {
	// 	printf(" 0x%02X ", formattedData[i]);
	// }
	// printf("\n\n");

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
		isFinalByte = buffer[offset];
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
	uint8_t firstByte = *response;

	authenticationResponse->status.generalStatus		= OK_STATUS;
	authenticationResponse->status.versionStatus		= OK_STATUS;
	authenticationResponse->status.authenticationStatus = OK_STATUS;
	authenticationResponse->version						= VERSION;

	if (firstByte & GENERAL_STATUS_MASK) {
		authenticationResponse->status.generalStatus = ERROR_STATUS;

		if (firstByte & VERSION_STATUS_MASK) {
			authenticationResponse->status.versionStatus = ERROR_STATUS;
			authenticationResponse->version				 = getVersion(response);
		}
		if (firstByte & AUTH_STATUS_MASK) {
			authenticationResponse->status.authenticationStatus = ERROR_STATUS;
		}
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

int sendByeRequest(int server) {
	uint8_t byeRequest = BYE_MASK;

	return sendSCTPMsg(server, (void *) &byeRequest, 1, BYE_STREAM);
}

int sendGetRequest(int server, uint8_t id, timeTag_t timeTag) {
	size_t length		= sizeof(uint8_t) + sizeof(timeTag_t);
	uint8_t *getRequest = malloc(length);

	getRequest[0] = GET_MASK | (id & ID_MASK);

	memcpy(getRequest + sizeof(uint8_t), &timeTag, sizeof(timeTag));

	int sent = sendSCTPMsg(server, (void *) getRequest, length, GET_STREAM);

	// printf("[protocol.c][sendGetRequest] GET-REQUEST = "); // TODO LOGGER
	// for (int i = 0; i < length; i++) {
	// 	printf(" 0x%02X ", getRequest[i]);
	// }
	// printf("\n\n");

	free(getRequest);

	return sent;
}

int sendSetRequest(int server, uint8_t id, timeTag_t timeTag, void *data,
				   size_t dataLength) {
	size_t formattedDataLength;
	void *formattedData = formatData(data, dataLength, &formattedDataLength);
	size_t length = sizeof(uint8_t) + sizeof(timeTag_t) + formattedDataLength;
	uint8_t *setRequest = malloc(length);

	setRequest[0] = SET_MASK | (id & ID_MASK);

	memcpy(setRequest + sizeof(uint8_t), &timeTag, sizeof(timeTag));
	memcpy(setRequest + sizeof(uint8_t) + sizeof(timeTag_t), formattedData,
		   formattedDataLength);

	int sent = sendSCTPMsg(server, (void *) setRequest, length, SET_STREAM);

	// printf("[protocol.c][sendSetRequest] SET-REQUEST = "); // TODO LOGGER
	// for (int i = 0; i < length; i++) {
	// 	printf(" 0x%02X ", setRequest[i]);
	// }
	// printf("\n\n");

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

	if (response->status.operationStatus == OK_STATUS &&
		response->status.idStatus == OK_STATUS) {
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
	/* If timeTagStatus = OK_STATUS you already had the last version of the
	 * resource */
	if (response->status.timeTagStatus == OK_STATUS) {
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
	if (response->status.timeTagStatus == OK_STATUS) {
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
		headByte & GENERAL_STATUS_MASK ? ERROR_STATUS : OK_STATUS;

	response->status.operationStatus =
		headByte & OPCODE_STATUS_MASK ? ERROR_STATUS : OK_STATUS;

	response->status.idStatus =
		headByte & ID_STATUS_MASK ? ERROR_STATUS : OK_STATUS;

	response->status.timeTagStatus =
		headByte & TTAG_STATUS_MASK ? ERROR_STATUS : OK_STATUS;

	switch (response->streamNumber) {
		case GET_STREAM:
			response->operation = GET_OP;
			break;
		case SET_STREAM:
			response->operation = SET_OP;
			break;
		default:
			/* If reached, operationStatus must be ERROR */
			break;
	}

	return read;
}

int recvRequest(int client, request_t *request) {
	int totalRead = 0;
	int read;

	read = recvHeadByteAndLoadRequestInfo(client, request);

	if (read < 0) {
		return read;
	}

	totalRead += read;

	if (request->operationStatus == OK_STATUS) {
		switch (request->operation) {
			case BYE_OP:
				/* Nothing extra to recv */
				read = 0;
				break;
			case GET_OP:
				read = recvGetRequest(client, request);
				break;
			case SET_OP:
				read = recvSetRequest(client, request);
				break;
		}

		if (read < 0) {
			return read;
		}

		totalRead += read;
	}

	return totalRead;
}

static int recvGetRequest(int client, request_t *request) {
	request->data		= NULL;
	request->dataLength = 0;

	return recvTimeTag(client, &request->timeTag);
}

static int recvSetRequest(int client, request_t *request) {
	int read;
	int totalRead = 0;

	read = recvTimeTag(client, &request->timeTag);

	if (read < 0) {
		return read;
	}

	totalRead += read;

	request->data		= NULL;
	request->dataLength = 0;

	read = getConcretData(client, (uint8_t **) &request->data,
						  &request->dataLength);

	if (read < 0) {
		return read;
	}

	totalRead += read;

	return totalRead;
}

int recvAuthenticationRequest(int client, char **username, char **password,
							  uint8_t *hasSameVersion) {
	uint8_t *buffer = NULL;
	struct sctp_sndrcvinfo sndRcvInfo;
	int flags = 0;

	int read =
		receiveSCTPMsg(client, (void **) &buffer, 0, &sndRcvInfo, &flags);

	if (read < 0) {
		return read;
	}

	uint8_t versionByte = buffer[0];
	*hasSameVersion		= FALSE;

	if (versionByte == VERSION_BYTE) {
		*hasSameVersion = TRUE;
		size_t length;

		*username = allocateAndCopyString((char *) (buffer + 1), &length);

		if (*username == NULL) {
			return -1;
		}

		*password =
			allocateAndCopyString((char *) (buffer + 1 + length + 1), &length);

		if (*password == NULL) {
			return -1;
		}
	}

	free(buffer);

	return read;
}

/* Read first byte and loads information to request struct */
static int recvHeadByteAndLoadRequestInfo(int fd, request_t *request) {
	uint8_t headByte;
	int read = recvHeadByte(fd, &headByte, &request->streamNumber);

	if (read < 0) {
		return read;
	}

	request->operationStatus = OK_STATUS;

	switch (headByte & OPCODE_MASK) {
		case GET_MASK:
			request->operation = GET_OP;
			break;
		case SET_MASK:
			request->operation = SET_OP;
			break;
		case BYE_MASK:
			request->operation = BYE_OP;
			break;
		default:
			request->operationStatus = ERROR_STATUS;
			break;
	}

	request->id = headByte & ID_MASK;

	return read;
}

int sendAuthenticationResponse(
	int client, authenticationResponse_t authenticationResponse) {
	uint8_t response = 0x00;

	if (authenticationResponse.status.generalStatus != OK_STATUS) {
		response = response | GENERAL_STATUS_MASK;

		if (authenticationResponse.status.versionStatus != OK_STATUS) {
			response = response | VERSION_STATUS_MASK;

			/* >> 3 because one shift per status bit */
			response = response | (VERSION_BYTE >> 3);
		}

		if (authenticationResponse.status.authenticationStatus != OK_STATUS) {
			response = response | AUTH_STATUS_MASK;
		}
	}

	return sendSCTPMsg(client, (void *) &response, 1, AUTHENTICATION_STREAM);
}

static char *allocateAndCopyString(char *source, size_t *length) {
	*length				= 0;
	char *string		= NULL;
	char *reallocString = NULL;

	for (int i = 0; source[i] != 0; i++) {
		if (*length % ALLOC_BLOCK == 0) {
			reallocString =
				realloc(string, (*length + ALLOC_BLOCK) * sizeof(*string));

			if (reallocString == NULL) {
				if (string != NULL) {
					free(string);
				}

				errorMessage = "Can't realloc!";
				return NULL;
			}
			else {
				string = reallocString;
			}
		}

		string[(*length)++] = source[i];
	}

	if (*length % ALLOC_BLOCK == 0) {
		reallocString = realloc(string, (*length + 1) * sizeof(*string));

		if (reallocString == NULL) {
			if (string != NULL) {
				free(string);
			}

			errorMessage = "Can't realloc!";
			return NULL;
		}
		else {
			string = reallocString;
		}
	}

	string[*length] = 0;

	return string;
}

int sendResponse(int client, response_t response) {
	uint8_t headByte		   = getResponseHeadByte(response.status);
	size_t formattedDataLength = 0;
	void *formattedData		   = NULL;
	uint8_t needsTimeTag	   = FALSE;

	switch (response.operation) {
		case GET_OP:
			if (response.status.idStatus != ERROR_STATUS &&
				response.status.operationStatus != ERROR_STATUS &&
				response.status.timeTagStatus != OK_STATUS) {
				needsTimeTag  = TRUE;
				formattedData = formatData(response.data, response.dataLength,
										   &formattedDataLength);
			}
			break;
		case SET_OP:
			if (response.status.idStatus != ERROR_STATUS &&
				response.status.operationStatus != ERROR_STATUS &&
				response.status.timeTagStatus == OK_STATUS) {
				needsTimeTag = TRUE;
			}
			break;
		default:
			break;
	}

	size_t length = sizeof(headByte) + (needsTimeTag ? sizeof(timeTag_t) : 0) +
					formattedDataLength;

	uint8_t *msg = malloc(length);

	msg[0] = headByte;

	memcpy(msg + sizeof(headByte), &response.timeTag,
		   (needsTimeTag ? sizeof(timeTag_t) : 0));

	if (formattedData != NULL) {
		memcpy(msg + sizeof(headByte) + (needsTimeTag ? sizeof(timeTag_t) : 0),
			   formattedData, formattedDataLength);
	}

	// printf("[protocol.c][sendResponse] RESPONSE = "); // TODO LOGGER
	// for (int i = 0; i < length; i++) {
	// 	printf(" 0x%02X ", msg[i]);
	// }
	// printf("\n\n");

	return sendSCTPMsg(client, (void *) msg, length, response.streamNumber);
}

static uint8_t getResponseHeadByte(responseStatus_t status) {
	uint8_t headByte = 0x00;

	headByte =
		headByte |
		(status.generalStatus == ERROR_STATUS ? GENERAL_STATUS_MASK : 0x00);
	headByte =
		headByte |
		(status.operationStatus == ERROR_STATUS ? OPCODE_STATUS_MASK : 0x00);
	headByte =
		headByte | (status.idStatus == ERROR_STATUS ? ID_STATUS_MASK : 0x00);
	headByte = headByte |
			   (status.timeTagStatus == ERROR_STATUS ? TTAG_STATUS_MASK : 0x00);

	return headByte;
}

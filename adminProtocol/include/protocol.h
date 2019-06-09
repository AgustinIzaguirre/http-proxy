#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <string.h>
#include <errno.h>

#define AUTHENTICATION_STREAM 0
#define VERSION 0

#define MAX_ATTEMPTS 4 /* Used to setup SCTP Socket */

/* Response status masks */
#define GENERAL_STATUS_MASK 0x80
#define OPCODE_STATUS_MASK 0x40
#define TTAG_STATUS_MASK 0x20
#define ID_STATUS_MASK 0x10

/* Authentication response status masks */
/* GENERAL_STATUS_MASK same as above... */
#define VERSION_STATUS_MASK 0x40
#define AUTH_STATUS_MASK 0x20

#define IS_FINAL_BYTE 0x10
#define NOT_FINAL_BYTE 0x00
#define START_DATA_BYTE 0x80

#define RECV_BYTES 10 /* To use in sctp_recvmsg when want to read all bytes */
#define VERSION_BYTES 1
#define VERSION_BYTE 0x80

#define CHECK_FOR_ERROR(status)                                                \
	{                                                                          \
		if (status < 0) {                                                      \
			errorMessage = strerror(errno);                                    \
			return status;                                                     \
		}                                                                      \
	}

enum operation_t { BYE_OP, GET_OP, SET_OP };
typedef enum operation_t operation_t;

enum statusCode_t { OK, ERROR };
typedef enum statusCode_t statusCode_t;

typedef uint64_t timeTag_t;

typedef struct {
	statusCode_t generalStatus;
	statusCode_t operationStatus;
	statusCode_t timeTagStatus;
	statusCode_t idStatus;
} responseStatus_t;

typedef struct {
	responseStatus_t status;
	operation_t operation;
	uint8_t id;
	timeTag_t timeTag;
	void *data;
	size_t dataLength;
	uint16_t streamNumber;
} response_t;

typedef struct {
	statusCode_t generalStatus;
	statusCode_t versionStatus;
	statusCode_t authenticationStatus;
} versionStatus_t;

typedef struct {
	versionStatus_t status;
	uint64_t version;
} authenticationResponse_t;

int establishConnection(const char *serverIP, uint16_t serverPort,
						uint16_t streamQuantity);

int sendAuthenticationRequest(int server, char *username, size_t usernameLength,
							  char *password, size_t passwordLength);

int recvAuthenticationResponse(
	int server, authenticationResponse_t *authenticationResponse);

int sendByeRequest(uint16_t streamNumber);

int sendGetRequest(uint8_t id, timeTag_t timeTag, uint16_t streamNumber);

int sendPostRequest(uint8_t id, timeTag_t timeTag, void *data,
					size_t dataLength, uint16_t streamNumber);

int recvResponse(response_t *response);

char *getProtocolErrorMessage();

#endif

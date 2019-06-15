#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#define ALLOC_BLOCK 8
#define STREAM_QUANTITY 2
#define AUTHENTICATION_STREAM 0
#define BYE_STREAM 1
#define GET_STREAM 0
#define SET_STREAM 1

#define VERSION 0

#define TRUE 1
#define FALSE 0

#define MAX_ATTEMPTS 4 /* Used to setup SCTP Socket */

/* Response status masks */
#define GENERAL_STATUS_MASK 0x80
#define OPCODE_STATUS_MASK 0x40
#define ID_STATUS_MASK 0x20
#define TTAG_STATUS_MASK 0x10

/* Authentication response status masks */
/* GENERAL_STATUS_MASK same as above... */
#define VERSION_STATUS_MASK 0x40
#define AUTH_STATUS_MASK 0x20

#define IS_FINAL_BYTE 0x10
#define NOT_FINAL_BYTE 0x00
#define START_DATA_BYTE 0x80

#define RECV_BYTES 10 /* To use in sctp_recvmsg when want to read all bytes */
#define CONCRET_DATA_BLOCK_BYTES 8
#define INFO_BYTES 2
#define DATA_BLOCK_BYTES (CONCRET_DATA_BLOCK_BYTES + INFO_BYTES)
#define VERSION_BYTES 1
#define VERSION_BYTE 0x80

#define ID_MASK 0x3F
#define OPCODE_MASK 0xC0

#define BYE_MASK 0x00
#define GET_MASK 0x40
#define SET_MASK 0x80

#define CHECK_FOR_ERROR(status)                                                \
	{                                                                          \
		if (status < 0) {                                                      \
			errorMessage = strerror(errno);                                    \
			return status;                                                     \
		}                                                                      \
	}

enum operation_t { BYE_OP, GET_OP, SET_OP };
typedef enum operation_t operation_t;

enum statusCode_t { OK_STATUS, ERROR_STATUS };
typedef enum statusCode_t statusCode_t;

typedef uint64_t timeTag_t;

typedef struct {
	statusCode_t generalStatus;
	statusCode_t operationStatus;
	statusCode_t idStatus;
	statusCode_t timeTagStatus;
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
} authStatus_t;

typedef struct {
	authStatus_t status;
	uint64_t version;
} authenticationResponse_t;

typedef struct {
	statusCode_t operationStatus;
	operation_t operation;
	uint8_t id;
	timeTag_t timeTag;
	void *data;
	size_t dataLength;
	uint16_t streamNumber;
} request_t;

char *getProtocolErrorMessage();

int establishConnection(const char *serverIP, uint16_t serverPort);

int sendAuthenticationRequest(int server, char *username, size_t usernameLength,
							  char *password, size_t passwordLength);

int recvAuthenticationRequest(int client, char **username, char **password,
							  uint8_t *hasSameVersion);

int sendAuthenticationResponse(int client,
							   authenticationResponse_t authenticationResponse);

int recvAuthenticationResponse(
	int server, authenticationResponse_t *authenticationResponse);

int sendByeRequest(int server);

int sendGetRequest(int server, uint8_t id, timeTag_t timeTag);

int sendSetRequest(int server, uint8_t id, timeTag_t timeTag, void *data,
				   size_t dataLength);

int recvRequest(int client, request_t *request);

int recvResponse(int server, response_t *response);

int sendResponse(int client, response_t response);

int bindAndGetServerSocket(uint16_t port, char *ipFilter);

#endif

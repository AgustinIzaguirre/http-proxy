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

/* Used to setup SCTP Socket */
#define MAX_ATTEMPTS 4

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
#define START_DATA_BYTE 0x80

#define OK 0
#define ERROR 1

#define RCV_BLOCK 10 // TODO: check this later!
#define VERSION_BYTES 1
#define VERSION_BYTE 0x80

typedef uint64_t timeTag_t;

enum operation_t { BYE_OP, GET_OP, SET_OP };
typedef enum operation_t operation_t;

typedef struct {
	uint8_t generalState;
	uint8_t operationState;
	//	...
	uint8_t blahState;
} states_t;

typedef struct {
	states_t states;
	operation_t operation;
	uint8_t id;
	timeTag_t timeTag;
	void *data;
	size_t dataLength;
	uint16_t streamNumber;
} response_t;

int establishConnection(const char *serverIP, uint16_t serverPort,
						uint16_t streamQuantity);

void sendAuthenticationRequest(int server, char *username,
							   size_t usernameLength, char *password,
							   size_t passwordLength);

uint8_t recvAuthenticationResponse(int server);

void sendByeRequest(uint16_t streamNumber);

void sendGetRequest(uint8_t id, timeTag_t timeTag, uint16_t streamNumber);

void sendPostRequest(uint8_t id, timeTag_t timeTag, void *data,
					 size_t dataLength, uint16_t streamNumber);

void recvResponse(response_t *response);

char *getProtocolErrorMessage();

#endif

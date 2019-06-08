#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdlib.h>

#define MAX_ATTEMPTS 4
#define AUTHENTICATION_STREAM 0

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

#endif

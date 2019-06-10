#ifndef TRANSFORM_BODY_H
#define TRANSFORM_BODY_H

enum transformCommandErrors {
	TRANSFORM_COMMAND_OK = 0,
	PIPE_CREATION_ERROR,
	FORK_ERROR,
	EXEC_ERROR,
	SELECT_ERROR
};

struct transformBody {
	int writeToTransformFd;
	int readFromTransformFd;
};

/*
 * Resets read buffer
 */
void transformBodyInit(const unsigned state, struct selector_key *key);

unsigned transformBodyRead(struct selector_key *key);
unsigned transformBodyWrite(struct selector_key *key);

#endif
#ifndef TRANSFORM_BODY_H
#define TRANSFORM_BODY_H

#define LIMITATING_CHARS 4 //'\r' and '\n' at begining and end

enum transformCommandStatus {
	TRANSFORM_COMMAND_OK = 0,
	PIPE_CREATION_ERROR,
	FORK_ERROR,
	EXEC_ERROR,
	NONBLOCKING_ERROR,
	SELECT_ERROR
};

struct transformBody {
	int writeToTransformFd;
	int readFromTransformFd;
	unsigned commandStatus;
	char *chunkedData;
	buffer chunkedBuffer;
	uint8_t transformCommandExecuted;
	uint8_t transformFinished;
	uint8_t responseFinished;
	pid_t commandPid;
};

/*
 * Resets read buffer
 */
void transformBodyInit(const unsigned state, struct selector_key *key);

int executeTransformCommand(struct selector_key *key);
unsigned transformBodyRead(struct selector_key *key);
unsigned transformBodyWrite(struct selector_key *key);

unsigned standardOriginRead(struct selector_key *key);
unsigned readFromTransform(struct selector_key *key);
unsigned readFromOrigin(struct selector_key *key);
unsigned standardClientWrite(struct selector_key *key);
unsigned writeToTransform(struct selector_key *key);
unsigned writeToClient(struct selector_key *key);
unsigned setStandardFdInterests(struct selector_key *key);
unsigned setFdInterestsWithTransformerCommand(struct selector_key *key);
unsigned setErrorDoneFd(struct selector_key *key);
void initializeChunkedBuffer(struct transformBody *transformBody, int length);
void prepareChunkedBuffer(buffer *chunkBuffer, buffer *inbuffer);
void sentLastChunked(buffer *chunkBuffer);
void transformBodyDestroy(const unsigned state, struct selector_key *key);

// TODO comment all methods

#endif
#ifndef TRANSFORM_BODY_H
#define TRANSFORM_BODY_H

#include <unchunkParser.h>

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
	struct unchunkParser unchunkParser;
	int writeToTransformFd;
	int readFromTransformFd;
	unsigned commandStatus;
	uint8_t *chunkedData;
	buffer chunkedBuffer;
	uint8_t transformCommandExecuted;
	uint8_t transformFinished;
	uint8_t responseFinished;
	uint8_t transformSelectors;
	uint8_t lastChunkSent;
	pid_t commandPid;
};

/*
 * Resets read buffer and initialize transform body structure
 */
void transformBodyInit(const unsigned state, struct selector_key *key);

/*
 * Executes the transformer command and redirect its stdin,
 * stdout and stderr to pipes and specified error file also
 * registers pipes file descriptors to selector if exec success
 */
int executeTransformCommand(struct selector_key *key);

/*
 * General read handle function depending on wich file
 * descriptor awaken selector the function it calls
 */
unsigned transformBodyRead(struct selector_key *key);

/*
 * General write handle function depending on wich file
 * descriptor awaken selector the function it calls
 */
unsigned transformBodyWrite(struct selector_key *key);

/*
 * Read from origin and writes to write buffer when
 * it wont send to transform and will chunk
 */
unsigned standardOriginRead(struct selector_key *key);

/*
 * Read from origin and writes to write buffer when
 * it wont send to transform and wont chunk
 */
unsigned standardOriginReadWithoutChunked(struct selector_key *key);

/*
 * Read from transform command and writes to read buffer
 */
unsigned readFromTransform(struct selector_key *key);

/*
 * Read from origin and writes to write buffer
 * when it will send to transform
 */
unsigned readFromOrigin(struct selector_key *key);

/*
 * Read from chunked buffer and writes to client
 * message chunked
 */
unsigned standardClientWrite(struct selector_key *key);

/*
 * Read from chunked buffer and writes to client
 * message and does not chunk
 */
unsigned standardClientWriteWithoutChunked(struct selector_key *key);

/*
 * Writes to transform from write buffer
 */
unsigned writeToTransform(struct selector_key *key);

/*
 * Writes to transform unchunked message of write buffer
 */
unsigned writeToTransformChunked(struct selector_key *key);

/*
 * Write to client from chunk buffer after transformation
 * and chunked
 */
unsigned writeToClient(struct selector_key *key);

/*
 * Set file descriptors interests when it wont transform response
 */
unsigned setStandardFdInterests(struct selector_key *key);

/*
 * Set file descriptors interests when it wont transform response and also
 * won't chunk
 */
unsigned setStandardFdInterestsWithoutChunked(struct selector_key *key);

/*
 * Set file descriptors interests when response is transformed
 */
unsigned setFdInterestsWithTransformerCommand(struct selector_key *key);

/*
 * Sets every file descriptor used to OP_NOOP before finishing connection
 */
unsigned setErrorDoneFd(struct selector_key *key);

/*
 * Allocates memory for chunk buffer according to maximum posible size buffer
 * given http buffer size and also initialize its buffer
 */
void initializeChunkedBuffer(struct transformBody *transformBody, int length);

/*
 * Sets chunk buffer with number of bytes in hexa of the message followed
 * by \r\n(CRLF) and then the data of the message followed by \r\n(CRLF)
 */
void prepareChunkedBuffer(buffer *chunkBuffer, buffer *inbuffer);

/*
 * Sets chunk buffer with last chunk(0\r\n)
 */
void sentLastChunked(buffer *chunkBuffer);

/*
 * Frees allocated memory for chunk buffer
 */
void transformBodyDestroy(const unsigned state, struct selector_key *key);

#endif

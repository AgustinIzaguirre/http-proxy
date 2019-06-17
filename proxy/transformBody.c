#include <stdint.h>
#include <buffer.h>
#include <httpProxyADT.h>
#include <transformBody.h>
#include <configuration.h>
#include <handleParsers.h>
#include <http.h>
#include <signal.h>
#include <wait.h>
#include <stdio.h>
#include <errno.h>

static int getLength(buffer *buffer);

void transformBodyInit(const unsigned state, struct selector_key *key) {
	signal(SIGPIPE, SIG_IGN);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	unchunkParserInit(&(transformBody->unchunkParser), key);
	buffer_reset(getReadBuffer(GET_DATA(key)));
	int length = getLength(getReadBuffer(GET_DATA(key)));
	initializeChunkedBuffer(transformBody, length);
	transformBody->transformSelectors = FALSE;

	if (getTransformContent(GET_DATA(key))) {
		transformBody->commandStatus = executeTransformCommand(key);
	}
	transformBody->transformCommandExecuted = FALSE;
	transformBody->transformFinished		= FALSE;
	transformBody->responseFinished			= FALSE;
	transformBody->lastChunkSent			= FALSE;
}

void transformBodyDestroy(const unsigned state, struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	if (transformBody->chunkedData != NULL) {
		free(transformBody->chunkedData);
	}
}

unsigned transformBodyRead(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(state);
	unsigned ret;

	if (transformBody->chunkedData == NULL) {
		setErrorDoneFd(key);
		printf("Cannot allocate memory"); // TODO lucas logger de error
		ret = ERROR;
	}
	else if (!getTransformContent(state) ||
			 transformBody->commandStatus != TRANSFORM_COMMAND_OK ||
			 !transformBody->transformSelectors) {
		if (getIsChunked(state)) {
			ret = standardOriginReadWithoutChunked(key);
		}
		else {
			ret = standardOriginRead(key);
		}
	}
	else if (key->fd == transformBody->readFromTransformFd) {
		ret = readFromTransform(key);
	}
	else if (key->fd == getOriginFd(state)) {
		ret = readFromOrigin(key);
	}
	else {
		setErrorDoneFd(key);
		printf("unsupported file descriptor error\n"); // TODO lucas logger de
													   // error
		ret = ERROR;
	}
	return ret;
}

unsigned transformBodyWrite(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(state);
	unsigned ret;

	if (transformBody->chunkedData == NULL) {
		setErrorDoneFd(key);
		printf("Cannot allocate memory"); // TODO lucas logger de error
		ret = ERROR;
	}
	else if (!getTransformContent(state) ||
			 transformBody->commandStatus != TRANSFORM_COMMAND_OK ||
			 !transformBody->transformSelectors) {
		if (getIsChunked(state)) {
			ret = standardClientWriteWithoutChunked(key);
		}
		ret = standardClientWrite(key);
	}
	else if (key->fd == transformBody->writeToTransformFd) {
		if (getIsChunked(state)) {
			ret = writeToTransformChunked(key);
		}
		else {
			ret = writeToTransform(key);
		}
	}
	else if (key->fd == getClientFd(state)) {
		ret = writeToClient(key);
	}
	else {
		setErrorDoneFd(key);
		printf("Unsupported file descriptor error\n"); // TODO lucas logger de
													   // error
		ret = ERROR;
	}
	return ret;
}

unsigned standardOriginRead(struct selector_key *key) {
	buffer *inBuffer					= getWriteBuffer(GET_DATA(key));
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	unsigned ret;
	if (!buffer_can_write(inBuffer) && buffer_can_read(inBuffer) &&
		buffer_can_write(chunkBuffer)) {
		prepareChunkedBuffer(chunkBuffer, inBuffer);
		return setStandardFdInterests(key);
	}
	// if there is no space to read should write what i already read
	else if (!buffer_can_write(inBuffer)) {
		return setStandardFdInterests(key);
	}

	pointer   = buffer_write_ptr(inBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_write_adv(inBuffer, bytesRead);
		prepareChunkedBuffer(chunkBuffer, inBuffer);
		ret = setStandardFdInterests(key);
	}
	else if (bytesRead == 0) {
		transformBody->responseFinished = TRUE;

		if (!buffer_can_read(inBuffer)) {
			sentLastChunked(chunkBuffer);
		}
		else {
			prepareChunkedBuffer(chunkBuffer, inBuffer);
		}
		ret = setStandardFdInterests(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error6:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}

	return ret;
}

unsigned standardOriginReadWithoutChunked(struct selector_key *key) {
	buffer *inBuffer					= getWriteBuffer(GET_DATA(key));
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	unsigned ret;

	// if there is no space to read should write what i already read
	if (!buffer_can_write(inBuffer)) {
		return setStandardFdInterestsWithoutChunked(key);
	}

	pointer   = buffer_write_ptr(inBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_write_adv(inBuffer, bytesRead);
		ret = setStandardFdInterestsWithoutChunked(key);
	}
	else if (bytesRead == 0) {
		transformBody->responseFinished = TRUE;
		ret = setStandardFdInterestsWithoutChunked(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error6:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}

	return ret;
}

unsigned readFromTransform(struct selector_key *key) {
	buffer *inbuffer					= getReadBuffer(GET_DATA(key));
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	unsigned ret;

	// if there is no space to read should write what i already read
	if (!buffer_can_write(inbuffer)) {
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_write_ptr(inbuffer, &count);
	bytesRead = read(key->fd, pointer, count);

	if (bytesRead > 0) {
		buffer_write_adv(inbuffer, bytesRead);
		prepareChunkedBuffer(chunkBuffer, inbuffer);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else if (bytesRead == 0) {
		transformBody->transformFinished = TRUE;
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error5:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}

	if (transformBody->transformFinished && !buffer_can_read(chunkBuffer)) {
		// We read and sent everything that transform command outputs so we sent
		// last chunked
		sentLastChunked(chunkBuffer);
		transformBody->lastChunkSent = TRUE;
		ret = setFdInterestsWithTransformerCommand(key);
	}

	return ret;
}

unsigned readFromOrigin(struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	unsigned ret;

	// if there is no space to read should write what i already read
	if (!buffer_can_write(writeBuffer)) {
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_write_ptr(writeBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_write_adv(writeBuffer, bytesRead);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else if (bytesRead == 0) {
		transformBody->responseFinished = TRUE;
		if (!buffer_can_read(writeBuffer)) {
			close(transformBody->writeToTransformFd);
		}
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error4:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}

	return ret;
}

unsigned standardClientWrite(struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *writeBuffer					= &transformBody->chunkedBuffer;
	unsigned ret						= TRANSFORM_BODY;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(writeBuffer)) {
		return setStandardFdInterests(key);
	}

	pointer   = buffer_read_ptr(writeBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);
	int i	 = 0;
	while (i < bytesRead) {
		fprintf(stderr, "%c", pointer[i++]);
	}
	if (bytesRead > 0) {
		buffer_read_adv(writeBuffer, bytesRead);
		if (!buffer_can_read(writeBuffer) && transformBody->responseFinished &&
			!transformBody->lastChunkSent) {
			transformBody->lastChunkSent = TRUE;
			sentLastChunked(writeBuffer);
		}
		increaseTransferBytes(bytesRead);
		ret = setStandardFdInterests(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error3:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}

	if (transformBody->responseFinished && !buffer_can_read(writeBuffer)) {
		setErrorDoneFd(key);
		ret = DONE;
	}

	return ret;
}

unsigned standardClientWriteWithoutChunked(struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));
	unsigned ret						= TRANSFORM_BODY;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(writeBuffer)) {
		return setStandardFdInterestsWithoutChunked(key);
	}

	pointer   = buffer_read_ptr(writeBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(writeBuffer, bytesRead);
		increaseTransferBytes(bytesRead);
		ret = setStandardFdInterestsWithoutChunked(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error3:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}

	if (transformBody->responseFinished && !buffer_can_read(writeBuffer)) {
		setErrorDoneFd(key);
		ret = DONE;
	}

	return ret;
}

unsigned writeToTransform(struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *inbuffer					= getWriteBuffer(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;
	unsigned ret						= TRANSFORM_BODY;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(inbuffer)) {
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_read_ptr(inbuffer, &count);
	bytesRead = write(key->fd, pointer, count);

	if (bytesRead > 0) {
		if (transformBody->transformCommandExecuted == FALSE) {
			transformBody->transformCommandExecuted = TRUE;
		}
		buffer_read_adv(inbuffer, bytesRead);
		if (!buffer_can_read(inbuffer) && transformBody->responseFinished) {
			close(transformBody->writeToTransformFd);
		}
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else if (transformBody->transformCommandExecuted == TRUE) {
		setErrorDoneFd(key);
		printf("error2:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}
	else {
		transformBody->commandStatus = EXEC_ERROR;
		prepareChunkedBuffer(chunkBuffer, inbuffer);
		ret = setStandardFdInterests(key);
	}

	if (transformBody->responseFinished && !buffer_can_read(inbuffer)) {
		// closing pipe so that transform command finishes
		close(transformBody->writeToTransformFd);
	}

	return ret;
}

unsigned writeToTransformChunked(struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	struct unchunkParser *unchunkParser = &transformBody->unchunkParser;
	buffer *inbuffer					= getWriteBuffer(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;
	buffer *unchunkData					= &unchunkParser->unchunkedBuffer;
	unsigned ret						= TRANSFORM_BODY;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	parseChunkedInfo(unchunkParser, inbuffer);
	// if everything is read on buffer
	if (!buffer_can_read(unchunkData)) {
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_read_ptr(unchunkData, &count);
	bytesRead = write(key->fd, pointer, count);

	if (bytesRead > 0) {
		if (transformBody->transformCommandExecuted == FALSE) {
			transformBody->transformCommandExecuted = TRUE;
		}
		buffer_read_adv(unchunkData, bytesRead);
		if (!buffer_can_read(unchunkData) && transformBody->responseFinished) {
			close(transformBody->writeToTransformFd);
		}
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else if (transformBody->transformCommandExecuted == TRUE) {
		setErrorDoneFd(key);
		printf("error2:\n%s\n", strerror(errno)); // TODO lucas logger de error
		ret = ERROR;
	}
	else {
		transformBody->commandStatus = EXEC_ERROR;
		prepareChunkedBuffer(chunkBuffer, unchunkData);
		ret = setStandardFdInterests(key);
	}

	if (transformBody->responseFinished && !buffer_can_read(inbuffer)) {
		// closing pipe so that transform command finishes
		close(transformBody->writeToTransformFd);
	}

	return ret;
}

unsigned writeToClient(struct selector_key *key) {
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *buffer						= &transformBody->chunkedBuffer;
	unsigned ret						= TRANSFORM_BODY;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(buffer)) {
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_read_ptr(buffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	int i = 0;
	while (i < bytesRead) {
		fprintf(stderr, "%c", pointer[i++]);
	}
	if (bytesRead > 0) {
		buffer_read_adv(buffer, bytesRead);
		increaseTransferBytes(bytesRead);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error1:\n%s\n", strerror(errno)); // TODO lucas logger de error

		ret = ERROR;
	}

	if (transformBody->transformFinished && !buffer_can_read(buffer)) {
		if (!transformBody->lastChunkSent) {
			sentLastChunked(buffer);
		}
		setErrorDoneFd(key);
		ret = DONE;
	}

	return ret;
}
unsigned setStandardFdInterestsWithoutChunked(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));

	unsigned ret			   = TRANSFORM_BODY;
	int clientInterest		   = OP_NOOP;
	int originInterest		   = OP_NOOP;
	int transformReadInterest  = OP_NOOP;
	int transformWriteInterest = OP_NOOP;

	if (buffer_can_read(writeBuffer)) {
		clientInterest |= OP_WRITE;
	}

	if ((buffer_can_write(writeBuffer)) && !transformBody->responseFinished) {
		originInterest |= OP_READ;
	}

	if (SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getClientFd(state), clientInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getOriginFd(state), originInterest)) {
		return ERROR;
	}

	if (transformBody->transformSelectors) {
		if (SELECTOR_SUCCESS != selector_set_interest(
									key->s, transformBody->readFromTransformFd,
									transformReadInterest) ||
			SELECTOR_SUCCESS !=
				selector_set_interest(key->s, transformBody->writeToTransformFd,
									  transformWriteInterest)) {
			return ERROR;
		}
	}

	return ret;
}

unsigned setStandardFdInterests(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;

	unsigned ret			   = TRANSFORM_BODY;
	int clientInterest		   = OP_NOOP;
	int originInterest		   = OP_NOOP;
	int transformReadInterest  = OP_NOOP;
	int transformWriteInterest = OP_NOOP;

	if (buffer_can_read(chunkBuffer)) {
		clientInterest |= OP_WRITE;
	}

	if ((buffer_can_write(writeBuffer) || buffer_can_read(writeBuffer)) &&
		buffer_can_write(chunkBuffer) && !buffer_can_read(chunkBuffer) &&
		!transformBody->responseFinished) {
		originInterest |= OP_READ;
	}

	if (SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getClientFd(state), clientInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getOriginFd(state), originInterest)) {
		return ERROR;
	}

	if (transformBody->transformSelectors) {
		if (SELECTOR_SUCCESS != selector_set_interest(
									key->s, transformBody->readFromTransformFd,
									transformReadInterest) ||
			SELECTOR_SUCCESS !=
				selector_set_interest(key->s, transformBody->writeToTransformFd,
									  transformWriteInterest)) {
			return ERROR;
		}
	}

	return ret;
}

unsigned setFdInterestsWithTransformerCommand(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *readBuffer					= getReadBuffer(GET_DATA(key));
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;
	buffer *unchunkBuffer = &transformBody->unchunkParser.unchunkedBuffer;

	unsigned ret			   = TRANSFORM_BODY;
	int clientInterest		   = OP_NOOP;
	int originInterest		   = OP_NOOP;
	int transformReadInterest  = OP_NOOP;
	int transformWriteInterest = OP_NOOP;

	if (!getIsChunked(state)) {
		if (buffer_can_read(writeBuffer)) {
			transformWriteInterest |= OP_WRITE;
		}
	}
	else {
		if (buffer_can_read(unchunkBuffer) || buffer_can_read(writeBuffer)) {
			transformWriteInterest |= OP_WRITE;
		}
	}

	if (buffer_can_read(chunkBuffer)) {
		clientInterest |= OP_WRITE;
	}

	if (buffer_can_write(readBuffer) && !transformBody->transformFinished) {
		transformReadInterest |= OP_READ;
	}

	if (buffer_can_write(writeBuffer) && !transformBody->responseFinished &&
		!buffer_can_read(chunkBuffer) && !buffer_can_read(writeBuffer)) {
		originInterest |= OP_READ;
	}

	if (SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getClientFd(state), clientInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getOriginFd(state), originInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, transformBody->readFromTransformFd,
								  transformReadInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, transformBody->writeToTransformFd,
								  transformWriteInterest)) {
		return ERROR;
	}

	if (clientInterest == OP_NOOP && originInterest == OP_NOOP &&
		transformReadInterest == OP_NOOP && transformWriteInterest == OP_NOOP &&
		transformBody->responseFinished) {
		return DONE;
	}

	return ret;
}

unsigned setErrorDoneFd(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));

	unsigned ret			   = TRANSFORM_BODY;
	int clientInterest		   = OP_NOOP;
	int originInterest		   = OP_NOOP;
	int transformReadInterest  = OP_NOOP;
	int transformWriteInterest = OP_NOOP;

	if (SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getClientFd(state), clientInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getOriginFd(state), originInterest)) {
		return ERROR;
	}
	if (transformBody->transformSelectors) {
		if (SELECTOR_SUCCESS != selector_set_interest(
									key->s, transformBody->readFromTransformFd,
									transformReadInterest) ||
			SELECTOR_SUCCESS !=
				selector_set_interest(key->s, transformBody->writeToTransformFd,
									  transformWriteInterest)) {
			return ERROR;
		}
	}

	return ret;
}

int executeTransformCommand(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(state);
	int inputPipe[]						= {-1, -1};
	int outputPipe[]					= {-1, -1};
	int errorFd =
		open(getCommandStderrPath(getConfiguration()), OP_WRITE | OP_READ);
	char *commandPath = getCommand(getConfiguration());
	pid_t commandPid;

	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
		return PIPE_CREATION_ERROR;
	}

	commandPid = fork();
	if (commandPid == -1) {
		return FORK_ERROR;
	}
	else if (commandPid == 0) {
		dup2(inputPipe[0], 0);  // setting pipe as stdin
		dup2(outputPipe[1], 1); // setting pipe as stdout
		dup2(errorFd, 2);		// setting errorFd as stderr
		close(inputPipe[1]);	// closing write end of input pipe
		close(outputPipe[0]);   // closing read end of output pipe

		putenv("HTTPD_VERSION=1.0.0");
		if (execl("/bin/sh", "sh", "-c", commandPath, (char *) 0) == -1) {
			// closing other pipes end
			printf("In son\n");
			close(inputPipe[0]);
			close(outputPipe[1]);
			fprintf(stderr, "Exec error finishing\n");
			exit(-1);
		}
	}
	else {
		// In father process
		close(inputPipe[0]);  // closing read end of input pipe
		close(outputPipe[1]); // closing write end of output pipe
		if (selector_fd_set_nio(inputPipe[1]) == -1 ||
			selector_fd_set_nio(outputPipe[0]) == -1) {
			return NONBLOCKING_ERROR;
		}

		int status = 0;

		if (waitpid(commandPid, &status, WNOHANG) == commandPid) {
			return EXEC_ERROR;
		}

		transformBody->commandPid		   = commandPid;
		transformBody->writeToTransformFd  = inputPipe[1];
		transformBody->readFromTransformFd = outputPipe[0];

		if (SELECTOR_SUCCESS != selector_register(key->s, inputPipe[1],
												  getHttpHandler(), OP_WRITE,
												  state) ||
			SELECTOR_SUCCESS != selector_register(key->s, outputPipe[0],
												  getHttpHandler(), OP_READ,
												  state)) {
			return SELECT_ERROR;
		}

		transformBody->transformSelectors = TRUE;
		incrementReferences(state);
		incrementReferences(state);
	}
	return TRANSFORM_COMMAND_OK;
}

static int getLength(buffer *buffer) {
	int bufferLength = buffer->limit - buffer->write;
	int digits		 = getDigits(bufferLength, 10);

	return digits + bufferLength + LIMITATING_CHARS;
}

void initializeChunkedBuffer(struct transformBody *transformBody, int length) {
	transformBody->chunkedData = malloc(sizeof(char) * length);
	if (transformBody->chunkedData != NULL) {
		buffer_init(&transformBody->chunkedBuffer, length,
					transformBody->chunkedData);
	}
}

void prepareChunkedBuffer(buffer *chunkBuffer, buffer *inbuffer) {
	size_t count, bytes;
	uint8_t *writePointer, *readPointer;
	writePointer = buffer_write_ptr(inbuffer, &count);
	readPointer  = buffer_read_ptr(inbuffer, &count);
	bytes		 = writePointer - readPointer;

	writeNumber(chunkBuffer, bytes);
	buffer_write(chunkBuffer, '\r');
	buffer_write(chunkBuffer, '\n');

	while (bytes) {
		buffer_write(chunkBuffer, buffer_read(inbuffer));
		bytes--;
	}

	buffer_write(chunkBuffer, '\r');
	buffer_write(chunkBuffer, '\n');
}

void sentLastChunked(buffer *chunkBuffer) {
	writeNumber(chunkBuffer, 0);
	buffer_write(chunkBuffer, '\r');
	buffer_write(chunkBuffer, '\n');
}

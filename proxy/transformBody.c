#include <stdint.h>
#include <buffer.h>
#include <httpProxyADT.h>
#include <transformBody.h>
#include <configuration.h>
#include <handleParsers.h>
#include <http.h>
#include <signal.h>
#include <wait.h>

// TODO REMOVE HEADER
#include <stdio.h>
#include <errno.h>

static int getLength(buffer *buffer);

void transformBodyInit(const unsigned state, struct selector_key *key) {
	signal(SIGPIPE, SIG_IGN);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer_reset(getReadBuffer(GET_DATA(key)));
	int length = getLength(getReadBuffer(GET_DATA(key)));
	initializeChunkedBuffer(transformBody, length);
	transformBody->commandStatus = executeTransformCommand(key);
	transformBody->commandStatus = EXEC_ERROR;
	printf("arrived to transform body state\n"); // TODO remove
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
		printf("error9:\n%s\n", strerror(errno));
		ret = ERROR;
	}
	else if (transformBody->commandStatus != TRANSFORM_COMMAND_OK) {
		ret = standardOriginRead(key);
	}
	else if (key->fd == transformBody->readFromTransformFd) {
		ret = readFromTransform(key);
	}
	else if (key->fd == getOriginFd(state)) {
		ret = readFromOrigin(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error8:\n%s\n", strerror(errno));
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
		printf("error10:\n%s\n", strerror(errno));
		ret = ERROR;
	}
	else if (transformBody->commandStatus != TRANSFORM_COMMAND_OK) {
		ret = standardClientWrite(key);
	}
	else if (key->fd == transformBody->writeToTransformFd) {
		ret = writeToTransform(key);
	}
	else if (key->fd == getClientFd(state)) {
		ret = writeToClient(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error7:\n%s\n", strerror(errno));
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

	// if there is no space to read should write what i already read
	if (!buffer_can_write(inBuffer)) {
		// set interest no op on fd an write on origin fd
		return setStandardFdInterests(key);
	}

	pointer   = buffer_write_ptr(inBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_write_adv(inBuffer, bytesRead);
		prepareChunkedBuffer(chunkBuffer, inBuffer, bytesRead);
		ret = setStandardFdInterests(key);
	}
	else if (bytesRead == 0) {
		// if response is not chunked or is last chunk
		setErrorDoneFd(key);
		ret = DONE; // should send what is left on buffer TODO
	}
	else {
		setErrorDoneFd(key);
		printf("error6:\n%s\n", strerror(errno));
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
		// set interest no op on fd an write on origin fd
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_write_ptr(inbuffer, &count);
	bytesRead = read(key->fd, pointer, count);

	if (bytesRead > 0) {
		buffer_write_adv(inbuffer, bytesRead);
		prepareChunkedBuffer(chunkBuffer, inbuffer, bytesRead);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else if (bytesRead == 0) {
		// if response is not chunked or is last chunk
		setErrorDoneFd(key);
		ret = DONE; // should send what is left on buffer TODO
	}
	else {
		setErrorDoneFd(key);
		printf("error5:\n%s\n", strerror(errno));
		ret = ERROR;
	}

	return ret;
}

unsigned readFromOrigin(struct selector_key *key) {
	buffer *writeBuffer = getWriteBuffer(GET_DATA(key));
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	unsigned ret;

	// if there is no space to read should write what i already read
	if (!buffer_can_write(writeBuffer)) {
		// set interest no op on fd an write on origin fd
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_write_ptr(writeBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_write_adv(writeBuffer, bytesRead);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else if (bytesRead == 0) {
		// if response is not chunked or is last chunk
		setErrorDoneFd(key);
		ret = DONE; // should send what is left on buffer TODO
	}
	else {
		setErrorDoneFd(key);
		printf("error4:\n%s\n", strerror(errno));
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
		// set interest no op on fd an read on client fd
		return setStandardFdInterests(key);
	}

	pointer   = buffer_read_ptr(writeBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(writeBuffer, bytesRead);
		ret = setStandardFdInterests(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error3:\n%s\n", strerror(errno));
		ret = ERROR;
	}

	return ret;
}

unsigned writeToTransform(struct selector_key *key) {
	buffer *buffer = getWriteBuffer(GET_DATA(key));
	unsigned ret   = TRANSFORM_BODY;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(buffer)) {
		// set interest no op on fd an read on client fd
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_read_ptr(buffer, &count);
	bytesRead = write(key->fd, pointer, count);

	if (bytesRead > 0) {
		buffer_read_adv(buffer, bytesRead);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else {
		setErrorDoneFd(key);
		printf("error2:\n%s\n", strerror(errno));
		ret = ERROR;
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
		// set interest no op on fd an read on client fd
		return setFdInterestsWithTransformerCommand(key);
	}

	pointer   = buffer_read_ptr(buffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(buffer, bytesRead);
		ret = setFdInterestsWithTransformerCommand(key);
	}
	else {
		setErrorDoneFd(key);

		printf("error1:\n%s\n", strerror(errno));

		ret = ERROR;
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

	if (buffer_can_write(writeBuffer) && buffer_can_write(chunkBuffer)) {
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

	return ret;
}

unsigned setFdInterestsWithTransformerCommand(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(GET_DATA(key));
	buffer *readBuffer					= getReadBuffer(GET_DATA(key));
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));
	buffer *chunkBuffer					= &transformBody->chunkedBuffer;

	unsigned ret			   = TRANSFORM_BODY;
	int clientInterest		   = OP_NOOP;
	int originInterest		   = OP_NOOP;
	int transformReadInterest  = OP_NOOP;
	int transformWriteInterest = OP_NOOP;

	if (buffer_can_read(writeBuffer)) {
		transformWriteInterest |= OP_WRITE;
	}

	if (buffer_can_read(chunkBuffer)) {
		clientInterest |= OP_WRITE;
	}

	if (buffer_can_write(readBuffer)) {
		transformReadInterest |= OP_READ;
	}

	if (buffer_can_write(writeBuffer)) {
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
			selector_set_interest(key->s, getOriginFd(state), originInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, transformBody->readFromTransformFd,
								  transformReadInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, transformBody->writeToTransformFd,
								  transformWriteInterest)) {
		return ERROR;
	}

	return ret;
}

int executeTransformCommand(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(state);
	int inputPipe[]						= {-1, -1};
	int outputPipe[]					= {-1, -1};
	int errorFd =
		-1; // TODO open /dev/null or the setted in config and dup2 stderr
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
		// close(inputPipe[0]);	// closing unused copy of pipe
		// close(outputPipe[1]);   // closing unused copy o pipe
		close(inputPipe[1]);  // closing write end of input pipe
		close(outputPipe[0]); // closing read end of output pipe

		if (execl("/bin/sh", "sh", "-c", commandPath, (char *) 0) == -1) {
			// closing other pipes end
			printf("In son\n");
			close(inputPipe[0]);
			close(outputPipe[1]);
			fprintf(stderr, "Exec error finishing\n");
			exit(-1);
			// return EXEC_ERROR;
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
		//		sleep(5);
		//        pid_t answer = waitpid(commandPid, &status, WNOHANG);
		//        printf("pid: %d\n answer: %d\n", commandPid, answer);
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
		incrementReferences(state);
		incrementReferences(state);
	}

	return TRANSFORM_COMMAND_OK;
}

static int getLength(buffer *buffer) {
	int bufferLength = buffer->limit - buffer->write;
	int digits		 = getDigits(bufferLength);

	return digits + bufferLength + LIMITATING_CHARS;
}

void initializeChunkedBuffer(struct transformBody *transformBody, int length) {
	transformBody->chunkedData = malloc(sizeof(char) * length);
	if (transformBody->chunkedData != NULL) {
		buffer_init(&transformBody->chunkedBuffer, length,
					transformBody->chunkedData);
	}
}

void prepareChunkedBuffer(buffer *chunkBuffer, buffer *inbuffer,
						  int bytesRead) {
	writeNumber(chunkBuffer, bytesRead);
	buffer_write(chunkBuffer, '\r');
	buffer_write(chunkBuffer, '\n');

	while (bytesRead) {
		buffer_write(chunkBuffer, buffer_read(inbuffer));
		bytesRead--;
	}
}

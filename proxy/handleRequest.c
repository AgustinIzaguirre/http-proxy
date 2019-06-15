#include <handleRequest.h>
#include <http.h>
#include <httpProxyADT.h>
#include <stdio.h>
#include <selector.h>
#include <configuration.h>
#include <headersParser.h>
#include <utilities.h>
#include <logger.h>

/*
 * Returns buffer to read from to write on originFd
 */
static buffer *getCurrentBuffer(httpADT_t state);

void requestInit(const unsigned state, struct selector_key *key) {
	struct handleRequest *handleRequest = getHandleRequestState(GET_DATA(key));
	headersParserInit(&(handleRequest->parseHeaders), key, TRUE);
	addEntryToLog(GET_DATA(key), ACCESS_LOG);
	handleRequest->requestState = FIRST_BUFFER;
}

unsigned requestRead(struct selector_key *key) {
	buffer *readBuffer					= getReadBuffer(GET_DATA(key));
	struct handleRequest *handleRequest = getHandleRequestState(GET_DATA(key));
	unsigned ret						= HANDLE_REQUEST;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (key->fd == getOriginFd(GET_DATA(key))) {
		return HANDLE_RESPONSE;
	}

	// if there is no space to read should write what i already read
	if (!buffer_can_write(readBuffer)) {
		// set interest no op on fd an write on origin fd
		return setAdecuateFdInterests(key);
	}

	pointer   = buffer_write_ptr(readBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		int begining = pointer - readBuffer->data;
		buffer_write_adv(readBuffer, bytesRead);
		if (handleRequest->parseHeaders.state != BODY_START) {
			parseHeaders(&handleRequest->parseHeaders, readBuffer, begining,
						 begining + bytesRead);
		}

		ret = setAdecuateFdInterests(key);
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned requestWrite(struct selector_key *key) {
	buffer *readBuffer					= getCurrentBuffer(GET_DATA(key));
	struct handleRequest *handleRequest = getHandleRequestState(GET_DATA(key));
	unsigned ret						= HANDLE_REQUEST;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (handleRequest->requestState == FIRST_BUFFER &&
		!buffer_can_read(&handleRequest->parseHeaders.valueBuffer) &&
		handleRequest->parseHeaders.state != BODY_START) {
		parseHeaders(&handleRequest->parseHeaders,
					 getFinishParserBuffer(GET_DATA(key)), 0, 0);
		if (!buffer_can_read(getFinishParserBuffer(GET_DATA(key)))) {
			handleRequest->requestState = LAST_BUFFER;
		}
	}

	// if everything is read on buffer
	if (!buffer_can_read(readBuffer)) {
		// set interest no op on fd an read on client fd
		return setAdecuateFdInterests(key);
	}

	pointer   = buffer_read_ptr(readBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(readBuffer, bytesRead);
		increaseTransferBytes(bytesRead);
		ret = setAdecuateFdInterests(key);
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned setAdecuateFdInterests(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct handleRequest *handleRequest = getHandleRequestState(state);
	buffer *finishBuffer				= getFinishParserBuffer(state);
	buffer *readBuffer					= getReadBuffer(state);
	buffer *parsedBuffer = &(handleRequest->parseHeaders.valueBuffer);
	buffer *writeBuffer  = getWriteBuffer(state);
	unsigned ret		 = HANDLE_REQUEST;
	int clientInterest   = OP_NOOP;
	int originInterest   = OP_NOOP;

	if (buffer_can_write(readBuffer) && !buffer_can_read(parsedBuffer) &&
		!buffer_can_read(finishBuffer)) {
		clientInterest |= OP_READ;
	}

	if (buffer_can_write(writeBuffer)) {
		originInterest |= OP_READ;
	}

	if (buffer_can_read(parsedBuffer) ||
		(!handleRequest->parseHeaders.censure && buffer_can_read(readBuffer)) ||
		buffer_can_read(finishBuffer)) {
		originInterest |= OP_WRITE;
	}

	if (SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getClientFd(state), clientInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getOriginFd(state), originInterest)) {
		return ERROR;
	}

	return ret;
}

static buffer *getCurrentBuffer(httpADT_t state) {
	struct handleRequest *handleRequest = getHandleRequestState(state);

	if (handleRequest->parseHeaders.state != BODY_START ||
		buffer_can_read(&handleRequest->parseHeaders.valueBuffer)) {
		return &handleRequest->parseHeaders.valueBuffer;
	}
	else if (handleRequest->parseHeaders.state == BODY_START &&
			 buffer_can_read(getFinishParserBuffer(state))) {
		return getFinishParserBuffer(state);
	}
	else {
		return getReadBuffer(state);
	}
}

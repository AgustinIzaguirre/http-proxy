#include <handleResponse.h>
#include <http.h>
#include <httpProxyADT.h>
#include <headersParser.h>
#include <stdio.h>
#include <selector.h>
#include <transformBody.h>
#include <configuration.h>

static buffer *getCurrentResponseBuffer(httpADT_t state);

void responseInit(const unsigned state, struct selector_key *key) {
	struct handleResponse *handleResponse =
		getHandleResponseState(GET_DATA(key));
	headersParserInit(&(handleResponse->parseHeaders));
}

unsigned responseRead(struct selector_key *key) {
	buffer *writeBuffer = getWriteBuffer(GET_DATA(key));
	struct handleResponse *handleResponse =
		getHandleResponseState(GET_DATA(key));
	unsigned ret = HANDLE_RESPONSE;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (handleResponse->parseHeaders.state == BODY_START &&
		getIsTransformationOn(getConfiguration()) &&
		1 /* Mime type is in filter list */) { // TODO add condition
		return TRANSFORM_BODY;
	}
	// if there is no space to read should write what i already read
	if (!buffer_can_write(writeBuffer)) {
		// set interest no op on fd an write on origin fd
		return setResponseFdInterests(key);
	}

	pointer   = buffer_write_ptr(writeBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		int begining = pointer - writeBuffer->data;
		buffer_write_adv(writeBuffer, bytesRead);
		if (handleResponse->parseHeaders.state != BODY_START) {
			parseHeaders(&handleResponse->parseHeaders, writeBuffer, begining,
						 begining + bytesRead);
		}
		ret = setResponseFdInterests(key);
	}
	else if (bytesRead == 0) {
		// if response is not chunked or is last chunk
		ret = DONE; // should send what is left on buffer TODO
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned responseWrite(struct selector_key *key) {
	buffer *writeBuffer = getCurrentResponseBuffer(GET_DATA(key));
	struct handleResponse *handleResponse =
		getHandleResponseState(GET_DATA(key));
	unsigned ret = HANDLE_RESPONSE;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (handleResponse->parseHeaders.state == BODY_START &&
		getIsTransformationOn(getConfiguration()) &&
		1 /* Mime type is in filter list */) { // TODO add condition
		return TRANSFORM_BODY;
	}
	// if everything is read on buffer
	if (!buffer_can_read(writeBuffer)) {
		// set interest no op on fd an read on client fd
		return setResponseFdInterests(key);
	}

	pointer   = buffer_read_ptr(writeBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(writeBuffer, bytesRead);
		ret = setResponseFdInterests(key);
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned setResponseFdInterests(struct selector_key *key) {
	httpADT_t state						  = GET_DATA(key);
	struct handleResponse *handleResponse = getHandleResponseState(state);
	buffer *writeBuffer					  = getWriteBuffer(GET_DATA(key));
	buffer *parsedBuffer = &(handleResponse->parseHeaders.valueBuffer);
	;
	unsigned ret	   = HANDLE_RESPONSE;
	int clientInterest = OP_NOOP;
	int originInterest = OP_NOOP;

	if (buffer_can_read(parsedBuffer) ||
		(handleResponse->parseHeaders.state == BODY_START &&
		 buffer_can_read(writeBuffer))) {
		clientInterest |= OP_WRITE;
	}

	if (buffer_can_write(writeBuffer) && !buffer_can_read(parsedBuffer)) {
		originInterest |= OP_READ;
	}

	if (SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getClientFd(state), clientInterest) ||
		SELECTOR_SUCCESS !=
			selector_set_interest(key->s, getOriginFd(state), originInterest)) {
		return ERROR;
	}

	return ret;
}

static buffer *getCurrentResponseBuffer(httpADT_t state) {
	struct handleResponse *handleResponse = getHandleResponseState(state);
	if (handleResponse->parseHeaders.state != BODY_START ||
		buffer_can_read(&handleResponse->parseHeaders.valueBuffer)) {
		return &handleResponse->parseHeaders.valueBuffer;
	}
	else {
		return getWriteBuffer(state);
	}
}
#include <handleResponseWithTransform.h>
#include <http.h>
#include <httpProxyADT.h>
#include <headersParser.h>
#include <stdio.h>
#include <selector.h>

static buffer *getCurrentResponseBuffer(httpADT_t state);

void responseWithTransformInit(const unsigned state, struct selector_key *key) {
	struct handleResponseWithTransform *handleResponseWithTransform =
		getHandleResponseWithTransformState(GET_DATA(key));
	headersParserInit(&(handleResponseWithTransform->parseHeaders));
}

unsigned responseWithTransformRead(struct selector_key *key) {
	buffer *writeBuffer = getWriteBuffer(GET_DATA(key));
	struct handleResponseWithTransform *handleResponseWithTransform =
		getHandleResponseWithTransformState(GET_DATA(key));
	unsigned ret = HANDLE_RESPONSE_WITH_TRANSFORMATION;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	printf("In transformation\n");

	// if there is no space to read should write what i already read
	if (!buffer_can_write(writeBuffer)) {
		// set interest no op on fd an write on origin fd
		return setResponseWithTransformFdInterests(key);
	}

	pointer   = buffer_write_ptr(writeBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		int begining = pointer - writeBuffer->data;
		buffer_write_adv(writeBuffer, bytesRead);
		if (handleResponseWithTransform->parseHeaders.state != BODY_START) {
			parseHeaders(&handleResponseWithTransform->parseHeaders,
						 writeBuffer, begining, begining + bytesRead);
		}
		ret = setResponseWithTransformFdInterests(key);
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

unsigned responseWithTransformWrite(struct selector_key *key) {
	buffer *writeBuffer = getCurrentResponseBuffer(GET_DATA(key));
	unsigned ret		= HANDLE_RESPONSE_WITH_TRANSFORMATION;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	// if everything is read on buffer
	if (!buffer_can_read(writeBuffer)) {
		// set interest no op on fd an read on client fd
		return setResponseWithTransformFdInterests(key);
	}

	pointer   = buffer_read_ptr(writeBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(writeBuffer, bytesRead);
		ret = setResponseWithTransformFdInterests(key);
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned setResponseWithTransformFdInterests(struct selector_key *key) {
	httpADT_t state						  = GET_DATA(key);
	struct handleResponse *handleResponse = getHandleResponseState(state);
	buffer *writeBuffer					  = getWriteBuffer(GET_DATA(key));
	buffer *parsedBuffer				  = getCurrentResponseBuffer(state);
	unsigned ret						  = HANDLE_RESPONSE_WITH_TRANSFORMATION;
	int clientInterest					  = OP_NOOP;
	int originInterest					  = OP_NOOP;

	if (buffer_can_read(parsedBuffer)) {
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
	buffer *buf = &(
		getHandleResponseWithTransformState(state)->parseHeaders.headerBuffer);

	if (buffer_can_read(buf)) {
		return buf;
	}
	else {
		return getWriteBuffer(state);
	}
}

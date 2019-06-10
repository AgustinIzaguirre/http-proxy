#include <handleResponseWithTransform.h>
#include <http.h>
#include <httpProxyADT.h>
#include <headersParser.h>
#include <stdio.h>
#include <selector.h>
#include <configuration.h>

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
	// printf("In transformation\n"); // TODO REMOVE WHEN Transformation is
	// completed and tested

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
		else if (1 /* Mime type is in filter list */) {
			executeTransformCommand(key); // TODO validate return and act in
										  // response
			// TODO here is where we should call the transformer
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
	httpADT_t state = GET_DATA(key);
	struct handleResponseWithTransform *handleResponse =
		getHandleResponseWithTransformState(state);
	buffer *writeBuffer  = getWriteBuffer(GET_DATA(key));
	buffer *parsedBuffer = &(handleResponse->parseHeaders.valueBuffer);

	unsigned ret	   = HANDLE_RESPONSE_WITH_TRANSFORMATION;
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
	struct handleResponseWithTransform *handleResponse =
		getHandleResponseWithTransformState(state);
	if (handleResponse->parseHeaders.state != BODY_START ||
		buffer_can_read(&handleResponse->parseHeaders.valueBuffer)) {
		return &handleResponse->parseHeaders.valueBuffer;
	}
	else {
		return getWriteBuffer(state);
	}
}

unsigned setFdInterestsWithTransformerCommand(struct selector_key *key) {
	httpADT_t state = GET_DATA(key);
	struct handleResponseWithTransform *handleResponse =
		getHandleResponseWithTransformState(state);
	buffer *readBuffer   = getReadBuffer(GET_DATA(key));
	buffer *writeBuffer  = getWriteBuffer(GET_DATA(key));
	buffer *parsedBuffer = &(handleResponse->parseHeaders.valueBuffer);

	unsigned ret		  = HANDLE_RESPONSE_WITH_TRANSFORMATION;
	int clientInterest	= OP_NOOP;
	int originInterest	= OP_NOOP;
	int transformInterest = OP_NOOP;
	buffer_reset(readBuffer);

	if (buffer_can_read(writeBuffer)) {
		transformInterest |= OP_WRITE;
	}

	if (buffer_can_read(readBuffer)) {
		clientInterest |= OP_WRITE;
	}

	if (buffer_can_write(readBuffer)) {
		transformInterest |= OP_READ;
	}

	if (buffer_can_write(writeBuffer)) {
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
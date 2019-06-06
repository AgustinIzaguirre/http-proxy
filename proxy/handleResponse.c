#include <handleResponse.h>
#include <http.h>
#include <httpProxyADT.h>
#include <headersParser.h>
#include <stdio.h>
#include <selector.h>

void responseInit(const unsigned state, struct selector_key *key) {
	struct headersParser *parseheaders = getHeadersParser(GET_DATA(key));
	initializeHeaderParser(&parseheaders);
}

unsigned responseRead(struct selector_key *key) {
	buffer *writeBuffer					= getWriteBuffer(GET_DATA(key));
	struct headersParser *headersParser = getHeadersParser(GET_DATA(key));
	unsigned ret						= HANDLE_RESPONSE;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if there is no space to read should write what i already read
	if (!buffer_can_write(writeBuffer)) {
		// set interest no op on fd an write on origin fd
		return setResponseFdInterests(key);
	}

	pointer   = buffer_write_ptr(writeBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		int begining = pointer - writeBuffer->data;
		parseHeaders(headersParser, writeBuffer, begining,
					 begining + bytesRead);
		buffer_write_adv(writeBuffer, bytesRead);
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
	buffer *writeBuffer = getWriteBuffer(GET_DATA(key));
	unsigned ret		= HANDLE_RESPONSE;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	printf("censure: %d\n", getHeadersParser(GET_DATA(key))->censure); // evans
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
	httpADT_t state		= GET_DATA(key);
	buffer *writeBuffer = getWriteBuffer(GET_DATA(key));
	unsigned ret		= HANDLE_RESPONSE;
	int clientInterest  = OP_NOOP;
	int originInterest  = OP_NOOP;

	if (buffer_can_read(writeBuffer)) {
		clientInterest |= OP_WRITE;
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
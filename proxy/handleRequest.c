#include <handleRequest.h>
#include <http.h>
#include <httpProxyADT.h>
#include <stdio.h>
#include <selector.h>

unsigned requestRead(struct selector_key *key) {
	buffer *readBuffer = getReadBuffer(GET_DATA(key));
	unsigned ret	   = HANDLE_REQUEST;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	if (key->fd == getOriginFd(GET_DATA(key))) {
		return RESPONSE_HANDLER;
	}

	// if there is no space to read should write what i already read
	if (!buffer_can_write(readBuffer)) {
		// set interest no op on fd an write on origin fd
		return setAdecuateFdInterests(key);
	}

	pointer   = buffer_write_ptr(readBuffer, &count);
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_write_adv(readBuffer, bytesRead);
		// check if request finished TODO
		ret = setAdecuateFdInterests(key);
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned requestWrite(struct selector_key *key) {
	buffer *readBuffer = getReadBuffer(GET_DATA(key));
	unsigned ret	   = HANDLE_REQUEST;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(readBuffer)) {
		// set interest no op on fd an read on client fd
		return setAdecuateFdInterests(key);
	}

	pointer   = buffer_read_ptr(readBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(readBuffer, bytesRead);
		// check if request finished TODO
		ret = setAdecuateFdInterests(key);
	}
	else {
		ret = ERROR;
	}

	return ret;
}

unsigned setAdecuateFdInterests(struct selector_key *key) {
	httpADT_t state		= GET_DATA(key);
	buffer *readBuffer  = getReadBuffer(GET_DATA(key));
	buffer *writeBuffer = getWriteBuffer(GET_DATA(key));
	unsigned ret		= HANDLE_REQUEST;
	int clientInterest  = OP_NOOP;
	int originInterest  = OP_NOOP;

	if (buffer_can_write(readBuffer)) {
		clientInterest |= OP_READ;
	}

	if (buffer_can_write(writeBuffer)) {
		originInterest |= OP_READ;
	}

	if (buffer_can_read(readBuffer)) {
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
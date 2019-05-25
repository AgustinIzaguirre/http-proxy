#include <handleRequest.h>
#include <http.h>
#include <httpProxyADT.h>
#include <stdio.h>

#include <selector.h>

unsigned requestRead(struct selector_key *key) {
	httpADT_t state	= GET_DATA(key);
	buffer *readBuffer = getReadBuffer(GET_DATA(key));
	unsigned ret	   = HANDLE_REQUEST;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// printf("para leer del cliente\nHANDLE_REQUEST: %d\n",
	// HANDLE_REQUEST);//evans 		printf("is set origin: %d\n",
	// IsFdSet(getOriginFd(state), key->s)); 		printf("is set client:
	// %d\n", IsFdSet(getClientFd(state), key->s));

	// if there is no space to read should write what i already read
	if (!buffer_can_write(readBuffer)) {
		// set interest no op on fd an write on origin fd
		if (SELECTOR_SUCCESS !=
				selector_set_interest(key->s, key->fd, OP_NOOP) &&
			SELECTOR_SUCCESS !=
				selector_set_interest(key->s, getOriginFd(state), OP_WRITE)) {
			return ERROR;
		}

		// printf("returns %d\n", ret);//evans
		return ret;
	}

	pointer = buffer_write_ptr(readBuffer, &count);
	// printf("count to write on buffer: %d\n\n", count);// evans
	bytesRead = recv(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		// check if request finished TODO
		buffer_write_adv(readBuffer, bytesRead);
		if (!buffer_can_write(readBuffer)) {
			// printf("se lleno para escribir en buffer\n");//evans
			// printf("\noriginFd = %d\n", getOriginFd(state));//evans
			// printf("keyFd = %d\n", key->fd);//evans

			// register no op on fd and write on origin fd
			if (SELECTOR_SUCCESS !=
					selector_set_interest(key->s, key->fd, OP_NOOP) &&
				SELECTOR_SUCCESS != selector_set_interest(
										key->s, getOriginFd(state), OP_WRITE)) {
				return ERROR;
			}

			// printf("returns %d\n", ret);//evans
			return ret;
		}

		printf("deberia entrar a escribir en el origin\n");
	}
	else {
		ret = ERROR;
	}

	// printf("returns %d\n", ret);//evans
	return ret;
}

unsigned requestWrite(struct selector_key *key) {
	httpADT_t state	= GET_DATA(key);
	buffer *readBuffer = getReadBuffer(GET_DATA(key));
	unsigned ret	   = HANDLE_REQUEST;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;
	// printf("para escribir en el origin\n");//evans
	// 	printf("is set origin: %d\n", IsFdSet(getOriginFd(state), key->s));
	// 	printf("is set client: %d\n", IsFdSet(getClientFd(state), key->s));

	// if everything is read on buffer
	if (!buffer_can_read(readBuffer)) {
		// set interest no op on fd an read on client fd
		if (SELECTOR_SUCCESS !=
				selector_set_interest(key->s, key->fd, OP_NOOP) &&
			SELECTOR_SUCCESS !=
				selector_set_interest(key->s, getClientFd(state), OP_READ)) {
			return ERROR;
		}

		printf("returns %d\n", ret); // evans
		return ret;
	}

	pointer = buffer_read_ptr(readBuffer, &count);
	printf("count to read on buffer: %d\n\n", count); // evans

	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		// check if request finished TODO
		buffer_read_adv(readBuffer, bytesRead);
		if (!buffer_can_read(readBuffer)) {
			// printf("\noriginFd = %d\n", getOriginFd(state));//evans
			// printf("keyFd = %d\n", key->fd);//evans

			// register no op on fd and read on client fd
			if (SELECTOR_SUCCESS !=
					selector_set_interest(key->s, key->fd, OP_NOOP) &&
				SELECTOR_SUCCESS != selector_set_interest(
										key->s, getClientFd(state), OP_READ)) {
				return ERROR;
			}

			// printf("returns %d\n", ret);//evans
			return ret;
		}
	}
	else {
		ret = ERROR;
	}

	// printf("returns %d\n", ret);//evans
	return ret;
}
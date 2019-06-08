#include <handleError.h>

// TODO: maybe put it in files
char *errorResposes[] = {"HTTP/1.0 400 Bad Request\n\
Content-Type: text/html; charset=UTF-8\n\
\n\
<!DOCTYPE html>\n\
<html lang=en>\n\
  <meta charset=utf-8>\n\
  <meta name=viewport content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n\
  <title>Error 400 (Bad Request)!!1</title>\n\
  <p><b>400.</b> <ins>That is the error.</ins>\n\
  <p>Your client has issued a malformed or illegal request.  <ins>That’s all we know.</ins>\n",

						 "HTTP/1.0 405 Method Not Allow\n\
Content-Type: text/html; charset=UTF-8\n\
\n\
<!DOCTYPE html>\n\
<html lang=en>\n\
  <meta charset=utf-8>\n\
  <meta name=viewport content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n\
  <title>Error 405 (Method Not Allow)!!1</title>\n\
  <p><b>405.</b> <ins>That is the error.</ins>\n\
  <p>The request method is inappropriate.  <ins>That’s all we know.</ins>\n",

						 "HTTP/1.0 505 HTTP Version Not Supported\n\
Content-Type: text/html; charset=UTF-8\n\
\n\
<!DOCTYPE html>\n\
<html lang=en>\n\
  <meta charset=utf-8>\n\
  <meta name=viewport content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n\
  <title>Error 505 (HTTP Version Not Supported)!!1</title>\n\
  <p><b>505.</b> <ins>That is the error.</ins>\n\
  <p>The request version of HTTP is not supported by the server.  <ins>That’s all we know.</ins>\n",

						 "HTTP/1.0 400 Bad Request\n\
Content-Type: text/html; charset=UTF-8\n\
\n\
<!DOCTYPE html>\n\
  <html lang=en>\n\
  <meta charset=utf-8>\n\
  <meta name=viewport content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n\
  <title>Error 400 (Bad Request)!!1</title>\n\
  <p><b>400.</b> <ins>That is the error.</ins>\n\
  <p>Your client has issued a malformed host or the host header was not the first header from the request.  <ins>That’s all we know.</ins>\n"};

void errorInit(const unsigned state, struct selector_key *key) {
	enum errorType errorTypeFound = getErrorType(GET_DATA(key));
	int sizeErrorMessage		  = strlen(errorResposes[errorTypeFound]);
	setErrorBuffer(GET_DATA(key), (uint8_t *) errorResposes[errorTypeFound],
				   sizeErrorMessage);
	buffer_write_adv(getErrorBuffer(GET_DATA(key)), sizeErrorMessage);
	selector_set_interest(key->s, getClientFd(GET_DATA(key)), OP_WRITE);
}

unsigned errorHandleWrite(struct selector_key *key) {
	buffer *writeBuffer = getErrorBuffer(GET_DATA(key));
	unsigned ret		= ERROR_CLIENT;
	uint8_t *pointer;
	size_t count;
	ssize_t bytesRead;

	// if everything is read on buffer
	if (!buffer_can_read(writeBuffer)) {
		return ERROR;
	}

	pointer   = buffer_read_ptr(writeBuffer, &count);
	bytesRead = send(key->fd, pointer, count, 0);

	if (bytesRead > 0) {
		buffer_read_adv(writeBuffer, bytesRead);
	}

	return ret;
}

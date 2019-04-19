/**
 * main.c - servidor proxy http concurrente
 *
 */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h> // socket
#include <sys/types.h>  // socket
#include <unistd.h>

#include <http.h>
#include <httpProxyADT.h>
#include <selector.h>

#define BACKLOG_QTY 20
#define ERROR -1

const int prepareTCPSocket(unsigned port, char *filterIP);

static bool done = false;

static void sigtermHandler(const int signal) {
	printf("signal %d, cleaning up and exiting\n", signal);
	done = true;
}

const char *errorMessage = NULL;

int main() {
	signal(SIGTERM, sigtermHandler); // handling SIGTERM
	signal(SIGINT, sigtermHandler);  // handling SIGINT
	close(0);						 // nothing to read from stdin
	unsigned port		   = 1234;
	selector_status ss	 = SELECTOR_SUCCESS;
	fd_selector selector   = NULL;
	const int serverSocket = prepareTCPSocket(port, NULL);

	if (serverSocket == ERROR) {
		goto finally;
	}

	// set management socket
	// const int managementSocket = prepareSCTPSocket(getManagementPort(
	//                              getConfiguration()), getFilterAdmin(
	//                              getConfiguration()));

	// if(managementSocket == ERROR) {
	//     goto finally;
	// }

	const struct selector_init conf = {
		.signal = SIGALRM,
		.select_timeout =
			{
				.tv_sec  = 10,
				.tv_nsec = 0,
			},
	};

	if (0 != selector_init(&conf)) {
		errorMessage = "initializing selector";
		goto finally;
	}

	selector = selector_new(1024);

	if (selector == NULL) {
		errorMessage = "unable to create selector";
		goto finally;
	}

	const struct fd_handler http = {
		.handle_read  = httpPassiveAccept,
		.handle_write = NULL,
		.handle_close = NULL, // nothing to free
	};

	ss = selector_register(selector, serverSocket, &http, OP_READ, NULL);
	if (ss != SELECTOR_SUCCESS) {
		errorMessage = "registering fd";
		goto finally;
	}

	while (!done) {
		errorMessage = NULL;
		ss			 = selector_select(selector);

		if (ss != SELECTOR_SUCCESS) {
			errorMessage = "serving";
			goto finally;
		}
	}

	if (errorMessage == NULL) {
		errorMessage = "closing";
	}

	int ret = 0;

finally:
	if (ss != SELECTOR_SUCCESS) {
		fprintf(stderr, "%s: %s\n", (errorMessage == NULL) ? "" : errorMessage,
				ss == SELECTOR_IO ? strerror(errno) : selector_error(ss));
		ret = 2;
	}
	else if (errorMessage) {
		perror(errorMessage);
		ret = 1;
	}

	if (selector != NULL) {
		selector_destroy(selector);
	}

	selector_close();
	httpPoolDestroy();

	if (serverSocket >= 0) {
		close(serverSocket);
	}

	return ret;
}

const int prepareTCPSocket(unsigned port, char *filterIP) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	if (filterIP != NULL) {
		addr.sin_addr.s_addr = inet_addr(filterIP);
	}
	else {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	const int currentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (currentSocket < 0) {
		errorMessage = "unable to create socket";
		return ERROR;
	}

	fprintf(stdout, "Listening on TCP port %d\n", port);

	// if server fails doesn't have to wait to reuse address
	setsockopt(currentSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	if (bind(currentSocket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		errorMessage = "unable to bind socket";
		return ERROR;
	}
	// evans I Think here we should filter IP not where we do filter IP

	if (listen(currentSocket, BACKLOG_QTY) < 0) {
		errorMessage = "unable to listen";
		return ERROR;
	}

	if (selector_fd_set_nio(currentSocket) == -1) {
		errorMessage = "getting server socket flags";
		return ERROR;
	}

	return currentSocket;
}

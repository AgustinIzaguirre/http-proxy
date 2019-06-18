/**
 * Concurrent HTTP Proxy Server
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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <http.h>
#include <httpProxyADT.h>
#include <selector.h>
#include <configuration.h>
#include <commandInterpreter.h>
#include <protocol.h>
#include <management.h>

#define BACKLOG_QTY 20
#define ERROR -1

const int prepareTCPSocket(unsigned port, char *filterInterface);

static bool done = false;

static void sigtermHandler(const int signal) {
	printf("Signal %d, cleaning up and exiting\n", signal);
	done = true;
}

static const char *errorMessage = NULL;

int main(const int argc, const char **argv) {
	/* Reads command line options and sets configurations */
	readOptions(argc, (char *const *) argv);
	/* Setting signals */
	signal(SIGTERM, sigtermHandler); /* Handling SIGTERM */
	signal(SIGINT, sigtermHandler);  /* Handling SIGINT */

	close(0); /* Nothing to read from stdin */
	unsigned proxyPort		  = getHttpPort(getConfiguration());
	unsigned managementPort   = getManagementPort(getConfiguration());
	char *httpInterface		  = getHttpInterfaces(getConfiguration());
	char *managementInterface = getManagementInterfaces(getConfiguration());
	selector_status ss		  = SELECTOR_SUCCESS;
	fd_selector selector	  = NULL;

	int httpSocketQty;
	int httpSockets[2];
	int managementSocketQty;
	char *managementInterfaces[2];
	int managementSockets[2];

	if (httpInterface == NULL) {
		httpSocketQty = 2;
		httpSockets[0] =
			prepareTCPSocket(proxyPort, DEFAULT_PROXY_IPV4_INTERFACE);
		httpSockets[1] =
			prepareTCPSocket(proxyPort, DEFAULT_PROXY_IPV6_INTERFACE);
	}
	else {
		httpSocketQty  = 1;
		httpSockets[0] = prepareTCPSocket(proxyPort, httpInterface);
	}

	for (int i = 0; i < httpSocketQty; i++) {
		if (httpSockets[i] == ERROR) {
			goto finally;
		}
	}

	if (managementInterface == NULL) {
		managementSocketQty		= 2;
		managementInterfaces[0] = DEFAULT_MANAGEMENT_IPV4_INTERFACE;
		managementInterfaces[1] = DEFAULT_MANAGEMENT_IPV6_INTERFACE;
		managementSockets[0] =
			bindAndGetServerSocket(managementPort, managementInterfaces[0]);
		managementSockets[1] =
			bindAndGetServerSocket(managementPort, managementInterfaces[1]);
	}
	else {
		managementSocketQty		= 1;
		managementInterfaces[0] = managementInterface;
		managementSockets[0] =
			bindAndGetServerSocket(managementPort, managementInterface);
	}

	for (int i = 0; i < managementSocketQty; i++) {
		if (managementSockets[i] < 0) {
			goto finally;
		}
	}

	for (int i = 0; i < managementSocketQty; i++) {
		if (listenManagementSocket(managementSockets[i], BACKLOG_QTY) < 0) {
			errorMessage = getManagementErrorMessage();
			goto finally;
		}

		fprintf(stdout,
				"Management: Listening on SCTP interface = %s port %d\n",
				managementInterfaces[i], managementPort);
	}

	initializeTimeTags();

	const struct selector_init conf = {
		.signal = SIGALRM,
		.select_timeout =
			{
				.tv_sec  = 10,
				.tv_nsec = 0,
			},
	};

	if (0 != selector_init(&conf)) {
		errorMessage = "Initializing selector";
		goto finally;
	}

	selector = selector_new(1024);

	if (selector == NULL) {
		errorMessage = "Unable to create selector";
		goto finally;
	}

	const struct fd_handler http = {.handle_read  = httpPassiveAccept,
									.handle_write = NULL,
									.handle_close = NULL, /* nothing to free */
									.handle_block = NULL};

	for (int i = 0; i < httpSocketQty; i++) {
		ss = selector_register(selector, httpSockets[i], &http, OP_READ, NULL);

		if (ss != SELECTOR_SUCCESS) {
			errorMessage = "Registering fd";
			goto finally;
		}
	}

	const struct fd_handler management = {
		.handle_read  = managementPassiveAccept,
		.handle_write = NULL,
		.handle_close = NULL, /* nothing to free */
		.handle_block = NULL};

	for (int i = 0; i < managementSocketQty; i++) {
		ss = selector_register(selector, managementSockets[i], &management,
							   OP_READ, NULL);

		if (ss != SELECTOR_SUCCESS) {
			errorMessage = "Registering fd";
			goto finally;
		}
	}

	while (!done) {
		errorMessage = NULL;
		ss			 = selector_select(selector);

		if (ss != SELECTOR_SUCCESS) {
			errorMessage = "Serving";
			goto finally;
		}
	}

	if (errorMessage == NULL) {
		errorMessage = "Closing";
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

	for (int i = 0; i < httpSocketQty; i++) {
		if (httpSockets[i] >= 0) {
			close(httpSockets[i]);
		}
	}

	return ret;
}

const int prepareTCPSocket(unsigned port, char *filterInterface) {
	struct sockaddr_storage *addr = calloc(1, sizeof(struct sockaddr_storage));

	if (inet_pton(AF_INET, filterInterface,
				  &(((struct sockaddr_in *) addr)->sin_addr.s_addr)) == 1) {
		addr->ss_family							  = AF_INET;
		((struct sockaddr_in *) addr)->sin_family = AF_INET;
		((struct sockaddr_in *) addr)->sin_port   = htons(port);
	}
	else if (inet_pton(AF_INET6, filterInterface,
					   &(((struct sockaddr_in6 *) addr)->sin6_addr.s6_addr)) ==
			 1) {
		addr->ss_family								= AF_INET6;
		((struct sockaddr_in6 *) addr)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *) addr)->sin6_port   = htons(port);
	}
	else {
		free(addr);
		errorMessage = "Invalid IP interface, fails to convert it";
		return -1;
	}

	const int currentSocket = socket(addr->ss_family, SOCK_STREAM, IPPROTO_TCP);

	if (currentSocket < 0) {
		free(addr);
		errorMessage = "Unable to create socket";
		return ERROR;
	}

	fprintf(stdout, "HTTP Proxy: Listening on TCP interface = %s port = %d\n",
			filterInterface, port);

	if (addr->ss_family == AF_INET6) {
		/* If AF_INET6, listen in IPv6 only */
		setsockopt(currentSocket, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1},
				   sizeof(int));
	}

	/* If server fails doesn't have to wait to reuse address */
	setsockopt(currentSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	if (bind(currentSocket, (struct sockaddr *) addr, sizeof(*addr)) < 0) {
		errorMessage = "Unable to bind socket";
		free(addr);
		return ERROR;
	}

	if (listen(currentSocket, BACKLOG_QTY) < 0) {
		errorMessage = "Unable to listen";
		free(addr);
		return ERROR;
	}

<<<<<<< HEAD
	if (selector_fd_set_nio(currentSocket) < 0) {
=======
	if (selector_fd_set_nio(currentSocket) == -1) {
>>>>>>> Fixed TODO's
		errorMessage = "Getting server socket flags";
		free(addr);
		return ERROR;
	}

	free(addr);

	return currentSocket;
}

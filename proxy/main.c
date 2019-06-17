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
	unsigned proxyPort		   = getHttpPort(getConfiguration());
	unsigned managementPort	= getManagementPort(getConfiguration());
	char *httpInterfaces	   = getHttpInterfaces(getConfiguration());
	char *managementInterfaces = getManagementInterfaces(getConfiguration());
	selector_status ss		   = SELECTOR_SUCCESS;
	fd_selector selector	   = NULL;
	const int serverSocket	 = prepareTCPSocket(proxyPort, httpInterfaces);

	if (serverSocket == ERROR) {
		goto finally;
	}

	/* Set management socket */
	const int managementSocket = bindAndGetServerSocket(
		managementPort, managementInterfaces == NULL ?
							DEFAULT_MANAGEMENT_INTERFACE :
							managementInterfaces);

	if (managementSocket < 0) {
		goto finally;
	}

	if (listenManagementSocket(managementSocket, BACKLOG_QTY) < 0) {
		errorMessage = getManagementErrorMessage();
		goto finally;
	}

	fprintf(stdout, "Management: Listening on SCTP port %d\n", managementPort);

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

	ss = selector_register(selector, serverSocket, &http, OP_READ, NULL);
	if (ss != SELECTOR_SUCCESS) {
		errorMessage = "Registering fd";
		goto finally;
	}

	const struct fd_handler management = {
		.handle_read  = managementPassiveAccept,
		.handle_write = NULL,
		.handle_close = NULL, /* nothing to free */
		.handle_block = NULL};

	ss = selector_register(selector, managementSocket, &management, OP_READ,
						   NULL);
	if (ss != SELECTOR_SUCCESS) {
		errorMessage = "Registering fd";
		goto finally;
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

	if (serverSocket >= 0) {
		close(serverSocket);
	}

	return ret;
}

const int prepareTCPSocket(unsigned port, char *interface) {
	char *filterInterface =
		interface == NULL ? DEFAULT_PROXY_INTERFACE : interface;
	int onlyIPv6 = interface == NULL ? 0 : 1;

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

	fprintf(stdout, "HTTP Proxy: Listening on TCP interface = %s%s port = %d\n",
			filterInterface, onlyIPv6 ? "" : " (IPv4 & IPv6)", port);

	if (addr->ss_family == AF_INET6) {
		/* If AF_INET6 and default interface, listen in IPv6 and IPv4 */
		/* Dual stack in one socket */
		setsockopt(currentSocket, IPPROTO_IPV6, IPV6_V6ONLY, &onlyIPv6,
				   sizeof(int));
	}

	/* If server fails doesn't have to wait to reuse address */
	setsockopt(currentSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	if (bind(currentSocket, (struct sockaddr *) addr, sizeof(*addr)) < 0) {
		errorMessage = "Unable to bind socket";
		return ERROR;
	}

	if (listen(currentSocket, BACKLOG_QTY) < 0) {
		errorMessage = "Unable to listen";
		return ERROR;
	}

	if (selector_fd_set_nio(currentSocket) < 0) {
		errorMessage = "Getting server socket flags";
		return ERROR;
	}

	return currentSocket;
}

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
#include <configuration.h>
#include <commandInterpreter.h>

#define BACKLOG_QTY 20
#define ERROR -1

const int prepareTCPSocket(unsigned port, char *filterInterface);
// const int prepareSCTPSocket(unsigned port, char *filterInterface);

void managementPassiveAccept(struct selector_key *key);

static bool done = false;

static void sigtermHandler(const int signal) {
	printf("signal %d, cleaning up and exiting\n", signal);
	done = true;
}

const char *errorMessage = NULL;

int main(const int argc, const char **argv) {
	// reads command line options and sets configurations
	readOptions(argc, (char *const *) argv);
	// setting signals
	signal(SIGTERM, sigtermHandler); // handling SIGTERM
	signal(SIGINT, sigtermHandler);  // handling SIGINT

	close(0); // nothing to read from stdin
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

	// set management socket
	const int managementSocket =
		prepareTCPSocket(managementPort, managementInterfaces);
	if (managementSocket == ERROR) {
		goto finally;
	}

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

	const struct fd_handler management = {
		.handle_read  = managementPassiveAccept,
		.handle_write = NULL,
		.handle_close = NULL, // nothing to free
	};

	ss = selector_register(selector, managementSocket, &management, OP_READ,
						   NULL);
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

const int prepareTCPSocket(unsigned port, char *filterInterface) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	if (filterInterface != NULL) {
		addr.sin_addr.s_addr = inet_addr(filterInterface);
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
	// TODO: I Think here we should filter IP not where we do filter IP

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

// const int prepareSCTPSocket(unsigned port, char *filterInterface) {
// 	struct sockaddr_in addr;
// 	struct sctp_initmsg initMsg;

//     memset(&addr, 0, sizeof(addr));

//     addr.sin_family      = AF_INET;
//     addr.sin_port        = htons(port);
// 	if(filterInterface != NULL) {
// 		addr.sin_addr.s_addr = inet_addr(filterInterface);
// 	}
// 	else {
// 		addr.sin_addr.s_addr = INADDR_ANY;
// 	}

//     const int currentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

// 	if(currentSocket < 0) {
//         errorMessage = "unable to create socket";
//         return ERROR;
//     }

//     fprintf(stdout, "Listening on TCP port %d\n", port);

//     // man 7 ip. no importa reportar nada si falla.
//     //if server fails doesn't have to wait to reuse address
//     setsockopt(currentSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1},
// 			   sizeof(int));

//     if(bind(currentSocket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
//         errorMessage = "unable to bind socket";
//         return ERROR;
//     }

// 	memset(&initMsg, 0, sizeof(initMsg));

// 	initMsg.sinit_num_ostreams 	= NUM_STREAMS;
// 	initMsg.sinit_max_instreams	= NUM_STREAMS;
// 	initMsg.sinit_max_attempts 	= MAX_ATTEMPTS;

// 	setsockopt(currentSocket, IPPROTO_SCTP, SCTP_INITMSG, &initMsg,
// 		 	   sizeof(initMsg));

// 	struct sctp_event_subscribe events;

//   	memset((void *) &events, 0, sizeof(events));

//    	/*	By default returns only the data read.
// 	   	To know which stream the data came we enable the data_io_event
// 	   	The info will be in sinfo_stream in sctp_sndrcvinfo struct */
// 	events.sctp_data_io_event = 1;
// 	setsockopt(currentSocket, SOL_SCTP, SCTP_EVENTS, (const void *) &events,
// 			   sizeof(events));

//     if(listen(currentSocket, BACKLOG_QTY) < 0) {
//         errorMessage = "unable to listen";
//         return ERROR;
//     }

// 	if(selectorFdSetNonBlocking(currentSocket) == -1) {
// 		errorMessage = "getting server socket flags";
// 		return ERROR;
// 	}

//     return currentSocket;
// }

void managementPassiveAccept(struct selector_key *key) {
	printf("management conection accepted\n");
}

#include <http.h>
#include <httpProxyADT.h>
#include <connectToOrigin.h>
#include <stdio.h>

int blockingToResolvName(struct selector_key *key, int fdClient) {
	httpADT_t currentState = GET_DATA(key);
	pthread_t tid;
	void **args = malloc(2 * sizeof(void *));
	args[0]		= currentState;
	args[1]		= malloc(sizeof(struct selector_key));
	memcpy(args[1], key, sizeof(struct selector_key));
	((struct selector_key *) (args[1]))->fd = fdClient;

	if (-1 == pthread_create(&tid, NULL,
							 (void *(*) (void *) )(addressResolvName),
							 (void *) args)) {
		return ERROR;
	}
	return 1;
}

void *addressResolvName(void **data) {
	pthread_detach(pthread_self());
	httpADT_t currentState   = data[0];
	struct selector_key *key = data[1];
	// resolv address evans todo
	selector_notify_block(key->s, key->fd);
	return (void *) 0;
}

unsigned addressResolvNameDone(struct selector_key *key) {
	struct addrinfo *res = NULL; // currentState->originResolution; evans

	return connectToOrigin(key, res);
}

int connectToOrigin(struct selector_key *key, struct addrinfo *ipEntry) {
	httpADT_t currentState = GET_DATA(key);
	setOriginPort(currentState, 1234); // should be already setted evans

	char *serverIp			  = "127.0.0.1"; // should be already setted evans
	unsigned short originPort = getOriginPort(currentState);
	int socketFd			  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (socketFd < 0) {
		fprintf(stderr, "socket() failed\n");
		return ERROR;
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr)); // Empty structure
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port   = htons(originPort);

	if (inet_pton(AF_INET, serverIp, &(serverAddr.sin_addr)) <= 0) {
		fprintf(stderr, "not valid IP\ninet_pton() failed\n");
		return ERROR;
	}

	if (connect(socketFd, (const struct sockaddr *) (&serverAddr),
				sizeof(serverAddr)) < 0) {
		fprintf(stderr, "connect() failed\n");
		return ERROR;
	}

	if (selector_fd_set_nio(socketFd) == -1) {
		return ERROR;
	}

	setOriginFd(currentState, socketFd);

	if (SELECTOR_SUCCESS != selector_register(key->s, socketFd,
											  getHttpHandler(), OP_WRITE,
											  currentState)) {
		return ERROR;
	}

	return HANDLE_REQUEST; // return new state now should abort
}
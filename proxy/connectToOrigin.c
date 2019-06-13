#include <http.h>
#include <httpProxyADT.h>
#include <connectToOrigin.h>
#include <stdio.h>
#include <selector.h>

int blockingToResolvName(struct selector_key *key, int fdClient) {
	if (SELECTOR_SUCCESS != selector_set_interest(key->s, fdClient, OP_NOOP)) {
		return ERROR;
	}
	httpADT_t currentState = GET_DATA(key);
	pthread_t tid;
	void **args = malloc(2 * sizeof(void *)); // TODO malloc
	args[0]		= currentState;
	args[1]		= malloc(sizeof(struct selector_key));
	memcpy(args[1], key, sizeof(struct selector_key));
	((struct selector_key *) (args[1]))->fd = fdClient;

	if (-1 == pthread_create(&tid, NULL,
							 (void *(*) (void *) )(addressResolvName),
							 (void *) args)) {
		return ERROR;
	}
	return CONNECT_TO_ORIGIN;
}

void *addressResolvName(void **data) {
	pthread_detach(pthread_self());
	httpADT_t currentState   = data[0];
	struct selector_key *key = data[1];

	setOriginResolutions(currentState, NULL);

	struct addrinfo hints, *res;
	int errcode;
	char *host = getOriginHost(currentState);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	errcode = getaddrinfo(host, NULL, &hints, &res);
	if (errcode != 0) {
		selector_notify_block(key->s, key->fd);
		return (void *) -1;
	}

	setOriginResolutions(currentState, res);

	selector_notify_block(key->s, key->fd);
	return (void *) 0;
}

unsigned addressResolvNameDone(struct selector_key *key) {
	int flag			 = ERROR_CLIENT;
	struct addrinfo *res = getOriginResolutions(GET_DATA(key));

	while (res && flag == ERROR_CLIENT) {
		flag = connectToOrigin(key, res);
		res  = res->ai_next;
	}

	if (flag == ERROR_CLIENT) {
		setErrorType(GET_DATA(key), NOT_FOUND_HOST);
	}

	return flag;
}

int connectToOrigin(struct selector_key *key, struct addrinfo *ipEntry) {
	httpADT_t currentState = GET_DATA(key);

	void *ptr;
	char serverIP[100];
	inet_ntop(ipEntry->ai_family, ipEntry->ai_addr->sa_data, serverIP, 100);

	switch (ipEntry->ai_family) {
		case AF_INET:
			ptr = &((struct sockaddr_in *) ipEntry->ai_addr)->sin_addr;
			break;
		case AF_INET6:
			ptr = &((struct sockaddr_in6 *) ipEntry->ai_addr)->sin6_addr;
			break;
	}
	inet_ntop(ipEntry->ai_family, ptr, serverIP, 100);

	unsigned short originPort = getOriginPort(currentState);
	int socketFd			  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (socketFd < 0) {
		return ERROR_CLIENT;
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr)); // Empty structure
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port   = htons(originPort);

	if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0) {
		return ERROR_CLIENT;
	}

	if (connect(socketFd, (const struct sockaddr *) (&serverAddr),
				sizeof(serverAddr)) < 0) {
		return ERROR_CLIENT;
	}

	if (selector_fd_set_nio(socketFd) == -1) {
		return ERROR_CLIENT;
	}

	setOriginFd(currentState, socketFd);

	if (SELECTOR_SUCCESS != selector_register(key->s, socketFd,
											  getHttpHandler(), OP_WRITE,
											  currentState)) {
		return ERROR_CLIENT;
	}
	incrementReferences(currentState);

	return HANDLE_REQUEST;
}

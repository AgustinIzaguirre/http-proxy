#include <handleRequest.h>
#include <stdio.h>

unsigned requestRead(struct selector_key *key) {
	printf("can read request from client\n");
	return 0;
}

unsigned requestWrite(struct selector_key *key) {
	printf("can write request on origin\n");
	return 0;
}
#include <stdint.h>
#include <buffer.h>
#include <httpProxyADT.h>
#include <transformBody.h>

void transformBodyInit(const unsigned state, struct selector_key *key) {
	// struct transformBody *transformBody =
	// getTransformBodyState(GET_DATA(key));
	printf("arrived to transform body state\n");
	buffer_reset(getReadBuffer(GET_DATA(key)));
}

unsigned transformBodyRead(struct selector_key *key) {
	return ERROR;
}
unsigned transformBodyWrite(struct selector_key *key) {
	return ERROR;
}
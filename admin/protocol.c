#include <protocol.h>
#include <netinet/sctp.h>

void sendSCTPMsg(int server, void *msg, size_t msgLength, int streamNumber) {
	sctp_sendmsg(server, msg, msgLength, NULL, 0, 0, 0, streamNumber, 0, 0);
}

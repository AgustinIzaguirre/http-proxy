#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <selector.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <protocol.h>
#include <unistd.h>
#include <time.h>
#include <metric.h>
#include <configuration.h>
#include <mediaRange.h>

#define ID_QUANTITY 9
#define ON 1
#define OFF 0

#define USERNAME "manager"
#define PASSWORD "pdc69"

enum resourceId_t {
	NO_ID,
	MIME_ID,
	CMD_ID,
	MTR_CN_ID,
	MTR_HS_ID,
	MTR_BT_ID,
	TF_ID
};
typedef enum resourceId_t resId_t;

typedef struct {
	uint8_t isAuthenticated;
	uint8_t authResponseSent;
	authenticationResponse_t authResponse;
	request_t request;
	response_t response;
} manager_t;

const char *getManagementErrorMessage();
timeTag_t generateAndUpdateTimeTag(uint8_t id);
int listenManagementSocket(int managementSocket, size_t backlogQuantity);
void managementPassiveAccept(struct selector_key *key);
void initializeTimeTags();

#endif

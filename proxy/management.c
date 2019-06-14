#include <management.h>

static const char *errorMessage = "";

int listenManagementSocket(int managementSocket, size_t backlogQuantity) {
	if (listen(managementSocket, backlogQuantity) < 0) {
		errorMessage = "Unable to listen";
		return -1;
	}

	if (selector_fd_set_nio(managementSocket) == -1) { // TODO: maybe < 0?
		errorMessage = "Getting server socket flags";
		return -1;
	}

	return 0;
}

const char *getManagementErrorMessage() {
	return errorMessage;
}

void managementPassiveAccept(struct selector_key *key) {
	// TODO:
}

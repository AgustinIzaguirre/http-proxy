#include <configuration.h>
#include <stdlib.h> //NULL
#include <netinet/in.h>

struct configuration {
	unsigned short httpPort;
	unsigned short managementPort;
	char *mediaTypes;
	char *command;
	int isTransformationOn;
	int commandStderrFd;
	char *filterHttp;
	char *filterAdmin;
};

static struct configuration config = {
	.httpPort			= DEFAULT_PROXY_HTTP_PORT,
	.managementPort		= DEFAULT_MANAGEMENT_PORT,
	.mediaTypes			= NULL,
	.command			= NULL,
	.commandStderrFd	= INVALID_FD,
	.isTransformationOn = FALSE,
	.filterHttp			= NULL,
	.filterAdmin		= NULL,
};

void initializeConfigBaseValues(configurationADT config) {
	config->commandStderrFd = open(STDERR_REDIRECT_DEFAULT, O_WRONLY);
}

configurationADT getConfiguration() {
	return &config;
}

int getCommandStderrFd(configurationADT config) {
	return config->commandStderrFd;
}

void setCommandStderrFd(configurationADT config, int errorFd) {
	config->commandStderrFd = errorFd;
}

unsigned short getHttpPort(configurationADT config) {
	return config->httpPort;
}

void setHttpPort(configurationADT config, unsigned short httpPort) {
	config->httpPort = httpPort;
}

unsigned short getManagementPort(configurationADT config) {
	return config->managementPort;
}

void setManagementPort(configurationADT config, unsigned short managementPort) {
	config->managementPort = managementPort;
}

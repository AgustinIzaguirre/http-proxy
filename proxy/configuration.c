#include <configuration.h>
#include <stdlib.h> //NULL
#include <netinet/in.h>
#include <utilities.h>

struct configuration {
	unsigned short httpPort;
	unsigned short managementPort;
	char *httpInterfaces;
	char *managementInterfaces;
	MediaRangePtr_t mediaRange;
	char *command;
	int isTransformationOn;
	int commandStderrFd;
	char *filterHttp;
	char *filterAdmin;
	char *commandStderrPath;
	char *filterManagement;
};

static struct configuration config = {
	.httpPort			  = DEFAULT_PROXY_HTTP_PORT,
	.managementPort		  = DEFAULT_MANAGEMENT_PORT,
	.httpInterfaces		  = NULL,
	.managementInterfaces = "127.0.0.1",
	.mediaRange			  = NULL,
	.command			  = NULL,
	.commandStderrFd	  = INVALID_FD,
	.commandStderrPath	= "/dev/null",
	.isTransformationOn   = FALSE,
	.filterHttp			  = NULL,
	.filterAdmin		  = NULL,
	.filterManagement	 = NULL,
};

void initializeConfigBaseValues(configurationADT config) {
	config->commandStderrFd = open(STDERR_REDIRECT_DEFAULT, O_WRONLY);
	config->mediaRange		= createMediaRange(";");
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

char *getCommandStderrPath(configurationADT config) {
	return config->commandStderrPath;
}

void setCommandStderrPath(configurationADT config, char *errorPath) {
	config->commandStderrPath = errorPath;
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

char *getHttpInterfaces(configurationADT config) {
	return config->httpInterfaces;
}

void setHttpInterfaces(configurationADT config, char *httpInterfaces) {
	config->httpInterfaces = httpInterfaces;
}

char *getManagementInterfaces(configurationADT config) {
	return config->managementInterfaces;
}

void setManagementInterfaces(configurationADT config,
							 char *managementInterfaces) {
	config->managementInterfaces = managementInterfaces;
}

void setCommand(configurationADT config, char *command) {
	config->command			   = command;
	config->isTransformationOn = TRUE;
}

char *getCommand(configurationADT config) {
	return config->command;
}

int getIsTransformationOn(configurationADT config) {
	return config->isTransformationOn;
}

void setMediaRange(configurationADT config, MediaRangePtr_t mediaRange) {
	config->mediaRange = mediaRange;
}

MediaRangePtr_t getMediaRange(configurationADT config) {
	return config->mediaRange;
}

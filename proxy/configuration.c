#include <configuration.h>
#include <stdlib.h> //NULL
#include <netinet/in.h>
#include <utilities.h>
#include <management.h>

struct configuration {
	unsigned short httpPort;
	unsigned short managementPort;
	char *httpInterfaces;
	char *managementInterfaces;
	MediaRangePtr_t mediaRange;
	char *command;
	uint8_t isTransformationOn;
	int commandStderrFd;
	char *commandStderrPath;
};

static struct configuration config = {
	.httpPort			  = DEFAULT_PROXY_HTTP_PORT,
	.managementPort		  = DEFAULT_MANAGEMENT_PORT,
	.httpInterfaces		  = NULL,
	.managementInterfaces = NULL,
	.mediaRange			  = NULL,
	.command			  = NULL,
	.commandStderrFd	  = INVALID_FD,
	.commandStderrPath	= "/dev/null",
	.isTransformationOn   = FALSE,
};

void initializeConfigBaseValues(configurationADT config) {
	config->commandStderrFd = open(STDERR_REDIRECT_DEFAULT, O_WRONLY);
	config->mediaRange		= createMediaRange(";");
}

configurationADT getConfiguration() {
	return &config;
}

void resetMediaRangeList(configurationADT config) {
	freeMediaRange(config->mediaRange);
	config->mediaRange = createMediaRange(";");

	generateAndUpdateTimeTag(MIME_ID);
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

void setCommandAndTransformations(configurationADT config, char *command) {
	int strLength = strlen(command);

	if (strLength > 0) {
		char *newCommand = malloc(strLength + 1);
		memcpy(newCommand, command, strLength + 1);
		config->command			   = newCommand;
		config->isTransformationOn = TRUE;
	}
	else {
		config->isTransformationOn = FALSE;
	}
}

void setTransformationState(configurationADT config, uint8_t state) {
	config->isTransformationOn = state;

	generateAndUpdateTimeTag(TF_ID);
}

uint8_t getIsTransformationOn(configurationADT config) {
	return config->isTransformationOn;
}

void setCommand(configurationADT config, char *command) {
	if (config->command != NULL) {
		free(config->command);
	}

	config->command = command;

	generateAndUpdateTimeTag(CMD_ID);
}

char *getCommand(configurationADT config) {
	char *command = config->command;

	if (command == NULL) {
		command = "";
	}

	return command;
}

void setMediaRange(configurationADT config, MediaRangePtr_t mediaRange) {
	config->mediaRange = mediaRange;
}

MediaRangePtr_t getMediaRange(configurationADT config) {
	return config->mediaRange;
}

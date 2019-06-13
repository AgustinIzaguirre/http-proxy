#ifndef LOGGER_H
#define LOGGER_H

#include <netinet/in.h>
/* Create a new log file */
void createLogFile();

/* Add a new entry (line) to an existing log file */
int addLogEntry();

/* Create a new log entry and save it in a buffer */
int createLogEntryStr(struct sockaddr_storage *clientAddr,
	char *logEntryBuffer);

int getIpAddresses(struct sockaddr_storage *clientAddr, char *host,
	char *server);

#endif

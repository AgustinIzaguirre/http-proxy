#ifndef LOGGER_H
#define LOGGER_H

#include <netinet/in.h>

#define FAILED 0
#define OK 1

/* Create a new log file */
void createLogFile();

/* Add a new entry (line) to an existing log file */
int addEntryToLog(struct sockaddr_storage *clientAddr, const char *logFileName);

/* Setters for the access log array of elements to construct an entry */
void setRemoteAddr(char **accessLog, const char *remoteAddr);
void setTime(char **accessLog);
void setRequest(char **accessLog, const char *request);
void setStatus(char **accessLog, const char *status);
void setHttpReferer(char **accessLog, const char *httpReferer);
void setHttpUserAgent(char **accessLog, const char *httpUserAgent);

#endif

#ifndef LOGGER_H
#define LOGGER_H

#include <netinet/in.h>
#include <httpProxyADT.h>

#define FAILED 0
#define OK 1

/* Create a new log file */
int createLogFile(const char *path);

/* Add a new entry (line) to an existing log file */
int addEntryToLog(httpADT_t http, const char *logFileName);

/* Setters for the access log array of elements to construct an entry */
void setRemoteAddr(char **accessLog, const char *remoteAddr);
void setTime(char **accessLog);
void setRequest(char **accessLog, const char *request);
void setStatus(char **accessLog, const char *status);
void setHttpReferer(char **accessLog, const char *httpReferer);
void setHttpUserAgent(char **accessLog, const char *httpUserAgent);

#endif

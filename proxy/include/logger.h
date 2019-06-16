#ifndef LOGGER_H
#define LOGGER_H

#include <netinet/in.h>
#include <httpProxyADT.h>

#define FAILED 0
#define OK 1

#define LOG_TYPES 2

enum logTypes { ACCESS_LOG, ERROR_LOG };
typedef enum logTypes log_t;

enum communicationTypes { REQ, RESP };
typedef enum communicationTypes communication_t;

/* Add a new entry (line) to an access log file called access.log */
int logAccess(httpADT_t http, communication_t action);

int logError(const char *errorMsg);

#endif

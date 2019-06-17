#ifndef LOGGER_H
#define LOGGER_H

#include <netinet/in.h>
#include <httpProxyADT.h>

#define FAILED 0
#define OK 1

#define LOG_TYPES 3

enum logTypes { ACCESS_LOG, DEBUG_LOG, ERROR_LOG };
typedef enum logTypes log_t;

enum communicationTypes { REQ, RESP };
typedef enum communicationTypes communication_t;

enum errorOptions { SYS_ERROR, CUSTOM_ERROR };
typedef enum errorOptions logError_t;

// TODO: What to do with the return values

/* Add a new entry (line) to an access log file called access.log */
void logAccess(httpADT_t http, communication_t action);

/* Add a new entry (line) to an error log file called error.log */
void logError(const char *errorMsg, logError_t errorType);

/* Add a new entry (line) to a debug log file called debug.log */
void logDebug(const char *debugMsg);

#endif

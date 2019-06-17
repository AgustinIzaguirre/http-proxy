#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <logger.h>
#include <stdio.h>
#include <time.h>
#include <http.h>

#define MAX_ENTRY_SIZE 512
#define MAX_ADDR_SIZE 128
#define MAX_PATH_SIZE 256

char **logFilesPaths = NULL;
char *client		 = NULL;
char *host			 = NULL;
char *dir			 = "./../logger/logs/";

int checkLogFileExistence(log_t type);
char *createAccessLogEntry(httpADT_t s, communication_t action);
char *createErrorLogEntry(const char *errorMsg, logError_t errorType);
char *createDebugLogEntry(const char *debugMsg);
int existsLogFilesArray();
void createLogFilesArray();
int existsLogFile(log_t logType);
int createLogFile(log_t type);
int createDir(const char *dir);
char *createPath(char *dir, log_t type);
char *writeTime(char *curr, const char *end);
char *getFirstLine(httpADT_t s, communication_t action);
int setClientAndHost(httpADT_t s, communication_t action);
char *writeClientAndHost(char *curr, const char *end, char *client, char *host,
						 communication_t action);
char *writeLogElemToLogEntry(char *curr, const char *end, const char *format,
							 const char *data);
void writeToLog(log_t type, const char *logEntry);

void logAccess(httpADT_t http, communication_t action) {
	char *logEntry = NULL;

	if (checkLogFileExistence(ACCESS_LOG) == FAILED) {
		return;
	}

	if ((logEntry = createAccessLogEntry(http, action)) == NULL) {
		return;
	}

	writeToLog(ACCESS_LOG, logEntry);
}

void logError(const char *errorMsg, logError_t errorType) {
	char *logEntry = NULL;

	if (checkLogFileExistence(ERROR_LOG) == FAILED) {
		return;
	}

	if ((logEntry = createErrorLogEntry(errorMsg, errorType)) == NULL) {
		return;
	}

	writeToLog(ERROR_LOG, logEntry);
}

void logDebug(const char *debugMsg) {
	char *logEntry = NULL;

	if (checkLogFileExistence(DEBUG_LOG) == FAILED) {
		return;
	}

	if ((logEntry = createDebugLogEntry(debugMsg)) == NULL) {
		return;
	}

	writeToLog(DEBUG_LOG, logEntry);
}

int checkLogFileExistence(log_t type) {
	if (!existsLogFilesArray()) {
		createLogFilesArray();
	}

	if (!existsLogFile(type)) {
		if (createLogFile(type) == FAILED) {
			return FAILED;
		}
	}

	return OK;
}

char *createAccessLogEntry(httpADT_t s, communication_t action) {
	char *beginning = malloc(MAX_ENTRY_SIZE);
	char *curr		= beginning;
	const char *end = curr + MAX_ENTRY_SIZE;
	char *firstLine = NULL;

	if (setClientAndHost(s, action) == FAILED) {
		return NULL;
	}

	if ((curr = writeTime(curr, end)) == NULL) {
		return NULL;
	}

	if ((curr = writeClientAndHost(curr, end, client, host, action)) == NULL) {
		return NULL;
	}

	firstLine = getFirstLine(s, action);

	if ((curr = writeLogElemToLogEntry(curr, end, "'%s'\n", firstLine)) ==
		NULL) {
		free(firstLine);
		return NULL;
	}

	free(firstLine);

	return curr < end ? beginning : NULL;
}

char *createErrorLogEntry(const char *errorMsg, logError_t errorType) {
	char *beginning = malloc(MAX_ENTRY_SIZE);
	char *curr		= beginning;
	char *end		= beginning + MAX_ENTRY_SIZE;

	if ((curr = writeTime(curr, end)) == NULL) {
		return NULL;
	}

	if (errorMsg == NULL || (strcmp(errorMsg, "") == 0)) {
		errorMsg = "Error";
	}

	if ((curr = writeLogElemToLogEntry(curr, end, "%s", errorMsg)) == NULL) {
		return NULL;
	}

	if (errorType == SYS_ERROR) {
		if ((curr = writeLogElemToLogEntry(curr, end, ": %s",
										   strerror(errno))) == NULL) {
			return NULL;
		}
	}

	if ((curr = writeLogElemToLogEntry(curr, end, "%s\n", "")) == NULL) {
		return NULL;
	}

	return curr < end ? beginning : NULL;
}

char *createDebugLogEntry(const char *debugMsg) {
	char *beginning = malloc(MAX_ENTRY_SIZE);
	char *curr		= beginning;
	char *end		= beginning + MAX_ENTRY_SIZE;

	if ((curr = writeTime(curr, end)) == NULL) {
		return NULL;
	}

	if ((curr = writeLogElemToLogEntry(curr, end, "%s\n", debugMsg)) == NULL) {
		return NULL;
	}

	return curr < end ? beginning : NULL;
}

int existsLogFilesArray() {
	return logFilesPaths != NULL;
}

void createLogFilesArray() {
	logFilesPaths = malloc(LOG_TYPES * sizeof(char *));

	for (int i = 0; i < LOG_TYPES; i++) {
		logFilesPaths[i] = NULL;
	}
}

int existsLogFile(log_t type) {
	return logFilesPaths[type] != NULL;
}

int createLogFile(log_t type) {
	char *newLogFilePath = NULL;

	if (createDir(dir) == FAILED) {
		return FAILED;
	}

	if ((newLogFilePath = createPath(dir, type)) == NULL) {
		return FAILED;
	}

	if (open(newLogFilePath, (O_RDWR | O_CREAT | O_APPEND | O_NONBLOCK),
			 0777) == -1) {
		fprintf(stderr,
				"[LOG] Error: failed attempting to create the log file\n");
		return FAILED;
	}

	logFilesPaths[type] = newLogFilePath;

	return OK;
}

int createDir(const char *dir) {
	struct stat st = {0};

	if (stat(dir, &st) == -1) {
		if (mkdir(dir, 0777) == -1) {
			fprintf(stderr, "[LOG] Error: failed attempting to create the "
							"missing logs directory\n");
			return FAILED;
		}
	}

	return OK;
}

char *createPath(char *dir, log_t type) {
	char *newLogFilePath = calloc(MAX_PATH_SIZE, sizeof(char *));
	const char *log =
		(type == ACCESS_LOG ? "access.log" :
							  (type == DEBUG_LOG ? "debug.log" : "error.log"));
	size_t bytesWritten = 0;

	if (dir == NULL) {
		dir = "./";
	}

	bytesWritten = snprintf(newLogFilePath, MAX_PATH_SIZE, "%s", dir);

	if (bytesWritten >= MAX_PATH_SIZE) {
		free(newLogFilePath);
		fprintf(stderr, "[LOG] Error: failed attempting to create the path to "
						"open/create the log file\n");
		return NULL;
	}

	bytesWritten = snprintf(newLogFilePath + strlen(dir),
							MAX_PATH_SIZE - strlen(dir), "%s", log);

	if (bytesWritten >= (MAX_PATH_SIZE - strlen(dir))) {
		fprintf(stderr, "[LOG] Error: failed attempting to create the path to "
						"open/create the log file\n");
		free(newLogFilePath);
		return NULL;
	}

	return newLogFilePath;
}

char *getTime() {
	time_t now = time(0);
	char *time = calloc(128, sizeof(char));
	snprintf(time, 128, "%s", ctime(&now));
	size_t len				 = strlen(time);
	time[len - strlen("\n")] = 0;
	return time;
}

char *writeTime(char *curr, const char *end) {
	return writeLogElemToLogEntry(curr, end, "[%s] ", getTime());
}

char *determineCommunication(httpADT_t s, communication_t action) {
	if (action == REQ) {
		return (char *) ((getRequestLineBuffer(s))->data);
	}
	else { // RESP
		return (char *) ((getResponseLineBuffer(s))->data);
	}
}

char *getFirstLine(httpADT_t s, communication_t action) {
	char *ret		= malloc(MAX_FIRST_LINE_LENGTH);
	char *firstLine = determineCommunication(s, action);
	size_t len		= 0;

	snprintf(ret, MAX_FIRST_LINE_LENGTH, "%s", firstLine);
	len						= strlen(ret);
	ret[len - strlen("\n")] = 0;

	return ret;
}

char *getIpAddress(struct sockaddr_in *addr, char *buff) {
	int bytesWritten =
		snprintf(buff, MAX_ADDR_SIZE, "%s", inet_ntoa(addr->sin_addr));

	if (bytesWritten >= MAX_ADDR_SIZE) {
		fprintf(stderr, "[LOG] Error: failed attempting to get the client "
						"and host addresses\n");
		return NULL;
	}

	return buff;
}

int setClientAndHost(httpADT_t s, communication_t action) {
	if (client == NULL) {
		client = malloc(MAX_ADDR_SIZE);
	}

	if (host == NULL) {
		host = malloc(MAX_ADDR_SIZE);
	}

	if (action == REQ) {
		if ((client = getIpAddress((struct sockaddr_in *) getClientAddress(s),
								   client)) == NULL) {
			return FAILED;
		}

		struct sockaddr *originIP = getOriginResolutions(s)->ai_addr;
		if ((host = getIpAddress((struct sockaddr_in *) originIP, host)) ==
			NULL) {
			return FAILED;
		}
	}

	return OK;
}

char *writeClientAndHost(char *curr, const char *end, char *client, char *host,
						 communication_t action) {
	char *aux = NULL;

	if (action == RESP) {
		aux	= client;
		client = host;
		host   = aux;
	}

	if ((curr = writeLogElemToLogEntry(curr, end, "[%s --> ", client)) ==
		NULL) {
		return NULL;
	}

	if ((curr = writeLogElemToLogEntry(curr, end, "%s] - ", host)) == NULL) {
		return NULL;
	}

	return curr;
}

char *writeLogElemToLogEntry(char *curr, const char *end, const char *format,
							 const char *data) {
	int bytesWritten = snprintf(curr, end - curr, format, data);

	if (bytesWritten >= (end - curr)) {
		fprintf(stderr, "[LOG] Error: failed attempting to create the log "
						"entry, the buffer ran out of space for the string\n");
		return NULL;
	}

	curr += bytesWritten;

	return curr;
}

void writeToLog(log_t type, const char *logEntry) {
	int logFileFd = 0;

	if ((logFileFd = open(logFilesPaths[type], (O_RDWR | O_APPEND | O_NONBLOCK),
						  0666)) == -1) {
		fprintf(stderr, "[LOG] Error: failed attempting to open the log file "
						"in order to write to it\n");
		return;
	}

	if (write(logFileFd, logEntry, strlen(logEntry)) < 0) {
		fprintf(stderr, "[LOG] Error: failed attempting to write a new entry "
						"to the log file\n");
	}

	if (close(logFileFd) == -1) {
		fprintf(stderr,
				"[LOG] Error: failed attempting to close the log file\n");
	}
}

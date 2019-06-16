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

// TODO: Is the accessLogElems enum even necessary?
enum accessLogElems {
	REMOTE_ADDR,
	TIME,
	REQUEST,
	STATUS,
	// TODO: BODY_BYTES_SENT,
	HTTP_REFERER,
	HTTP_USER_AGENT
};

char **logFilesPaths = NULL;
char *client		 = NULL;
char *host			 = NULL;
char *dir			 = "./logs/";

int checkLogFileExistence(log_t type);
char *createAccessLogEntry(httpADT_t s, communication_t action);
char *createErrorLogEntry(const char *errorMsg, logError_t errorType);
int writeToLog(log_t type, const char *logEntry);
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
// TODO: Handle FAILED returns

int logAccess(httpADT_t http, communication_t action) {
	char *logEntry = NULL;

	if (checkLogFileExistence(ACCESS_LOG) == FAILED) {
		return FAILED;
	}

	if ((logEntry = createAccessLogEntry(http, action)) == NULL) {
		return FAILED;
	}

	return writeToLog(ACCESS_LOG, logEntry);
}

int logError(const char *errorMsg, logError_t errorType) {
	char *logEntry = NULL;

	if (checkLogFileExistence(ERROR_LOG) == FAILED) {
		return FAILED;
	}

	if ((logEntry = createErrorLogEntry(errorMsg, errorType)) == NULL) {
		return FAILED;
	}

	return writeToLog(ERROR_LOG, logEntry);
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

	setClientAndHost(s, action);

	if ((curr = writeTime(curr, end)) == NULL) {
		return FAILED;
	}

	if ((curr = writeClientAndHost(curr, end, client, host, action)) == NULL) {
		return FAILED;
	}

	firstLine = getFirstLine(s, action);

	if ((curr = writeLogElemToLogEntry(curr, end, "'%s'\n", firstLine)) ==
		NULL) {
		free(firstLine);
		return FAILED;
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
		return FAILED;
	}

	logFilesPaths[type] = newLogFilePath;

	return OK;
}

int createDir(const char *dir) {
	struct stat st = {0};

	if (stat(dir, &st) == -1) {
		if (mkdir(dir, 0777) == -1) {
			return FAILED;
		}
	}

	return OK;
}

char *createPath(char *dir, log_t type) {
	char *newLogFilePath = calloc(MAX_PATH_SIZE, sizeof(char *));
	const char *log		 = (type == ACCESS_LOG ? "access.log" : "error.log");
	size_t bytesWritten  = 0;

	if (dir == NULL) {
		dir = "./";
	}

	bytesWritten = snprintf(newLogFilePath, MAX_PATH_SIZE, "%s", dir);

	if (bytesWritten >= MAX_PATH_SIZE) {
		free(newLogFilePath);
		return NULL;
	}

	bytesWritten = snprintf(newLogFilePath + strlen(dir),
							MAX_PATH_SIZE - strlen(dir), "%s", log);

	if (bytesWritten >= (MAX_PATH_SIZE - strlen(dir))) {
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

int getIpAddresses(struct sockaddr_storage *clientAddr, char *client,
				   char *host) {
	return getnameinfo((struct sockaddr *) clientAddr, sizeof(struct sockaddr),
					   client, MAX_ADDR_SIZE, host, MAX_ADDR_SIZE, NI_NAMEREQD);
}

int setClientAndHost(httpADT_t s, communication_t action) {
	if (client == NULL) {
		client = malloc(MAX_ADDR_SIZE);
	}

	if (host == NULL) {
		host = malloc(MAX_ADDR_SIZE);
	}

	if (action == REQ) {
		if (getIpAddresses(getClientAddress(s), client, NULL) != 0) {
			return FAILED;
		}

		host = getOriginHost(s);
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
		return NULL;
	}

	curr += bytesWritten;

	return curr;
}

int writeToLog(log_t type, const char *logEntry) {
	int logFileFd		 = 0;
	int couldWriteToFile = 0;

	if ((logFileFd = open(logFilesPaths[type], (O_RDWR | O_APPEND | O_NONBLOCK),
						  0666)) == -1) {
		return FAILED;
	}

	couldWriteToFile = (write(logFileFd, logEntry, strlen(logEntry)) >= 0);

	if (close(logFileFd) == -1) {
		return FAILED;
	}

	return couldWriteToFile;
}

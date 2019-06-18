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
#define MAX_TIME_SIZE 128

char **logFilesPaths = NULL;
char *client		 = NULL;
char *host			 = NULL;
char *dir			 = "./../logger/logs/";

int checkLogFileExistence(log_t type);
int createAccessLogEntry(char *beginning, httpADT_t s, communication_t action);
int createErrorLogEntry(char *beginning, char *errorMsg, logError_t errorType);
int createDebugLogEntry(char *beginning, const char *debugMsg);
int existsLogFilesArray();
void createLogFilesArray();
int existsLogFile(log_t logType);
int createLogFile(log_t type);
int createDir();
int createPath(char *newLogFilePath, log_t type);
int writeTime(char **curr, const char *end);
int writeFirstLine(httpADT_t s, communication_t action, char **curr,
				   const char *end);
int setClientAndHost(httpADT_t s, communication_t action);
int writeClientAndHost(char **curr, const char *end, char *client, char *host,
					   communication_t action);
int writeCustomError(char *errorMsg, char **curr, const char *beginning);
int writeSystemError(char **curr, const char *beginning, logError_t errorType);
int writeLogElemToLogEntry(char **curr, const char *end, const char *format,
						   const char *data);
void writeToLog(log_t type, const char *logEntry);

void logAccess(httpADT_t http, communication_t action) {
	if (checkLogFileExistence(ACCESS_LOG) == FAILED) {
		return;
	}

	char *logEntry = malloc(MAX_ENTRY_SIZE);

	if (createAccessLogEntry(logEntry, http, action) == FAILED) {
		free(logEntry);
		return;
	}

	writeToLog(ACCESS_LOG, logEntry);
	free(logEntry);
}

void logError(char *errorMsg, logError_t errorType) {
	if (checkLogFileExistence(ERROR_LOG) == FAILED) {
		return;
	}

	char *logEntry = malloc(MAX_ENTRY_SIZE);

	if (createErrorLogEntry(logEntry, errorMsg, errorType) == FAILED) {
		free(logEntry);
		return;
	}

	writeToLog(ERROR_LOG, logEntry);
	free(logEntry);
}

void logDebug(const char *debugMsg) {
	if (checkLogFileExistence(DEBUG_LOG) == FAILED) {
		return;
	}

	char *logEntry = malloc(MAX_ENTRY_SIZE);

	if (createDebugLogEntry(logEntry, debugMsg) == FAILED) {
		free(logEntry);
		return;
	}

	writeToLog(DEBUG_LOG, logEntry);
	free(logEntry);
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

int createAccessLogEntry(char *beginning, httpADT_t s, communication_t action) {
	if (setClientAndHost(s, action) == FAILED) {
		return FAILED;
	}

	char **curr		= &beginning;
	const char *end = beginning + MAX_ENTRY_SIZE;

	if (writeTime(curr, end) == FAILED) {
		return FAILED;
	}

	if (writeClientAndHost(curr, end, client, host, action) == FAILED) {
		return FAILED;
	}

	if (writeFirstLine(s, action, curr, end) == FAILED) {
		return FAILED;
	}

	return *curr < end ? OK : FAILED;
}

int createErrorLogEntry(char *beginning, char *errorMsg, logError_t errorType) {
	char **curr		= &beginning;
	const char *end = beginning + MAX_ENTRY_SIZE;

	if (writeTime(curr, end) == FAILED) {
		return FAILED;
	}

	if (writeCustomError(errorMsg, curr, beginning) == FAILED) {
		return FAILED;
	}

	if (writeSystemError(curr, beginning, errorType) == FAILED) {
		return FAILED;
	}

	return *curr < end ? OK : FAILED;
}

int createDebugLogEntry(char *beginning, const char *debugMsg) {
	char **curr		= &beginning;
	const char *end = beginning + MAX_ENTRY_SIZE;

	if (writeTime(curr, end) == FAILED) {
		return FAILED;
	}

	if (writeLogElemToLogEntry(curr, end, "%s\n", debugMsg) == FAILED) {
		return FAILED;
	}

	return *curr < end ? OK : FAILED;
}

int existsLogFilesArray() {
	return logFilesPaths != NULL;
}

void createLogFilesArray() {
	logFilesPaths = calloc(LOG_TYPES, sizeof(char *));
}

int existsLogFile(log_t type) {
	return logFilesPaths[type] != NULL;
}

int createLogFile(log_t type) {
	if (createDir() == FAILED) {
		return FAILED;
	}

	char *newLogFilePath = calloc(MAX_PATH_SIZE, sizeof(char *));

	if (createPath(newLogFilePath, type) == FAILED) {
		free(newLogFilePath);
		return FAILED;
	}

	if (open(newLogFilePath, (O_RDWR | O_CREAT | O_APPEND | O_NONBLOCK),
			 0777) == -1) {
		free(newLogFilePath);
		fprintf(stderr,
				"[LOG] Error: failed attempting to create the log file\n");
		return FAILED;
	}

	if (logFilesPaths[type] == NULL) {
		logFilesPaths[type] = calloc(MAX_PATH_SIZE, sizeof(char));
	}

	int couldCreateFile = snprintf(logFilesPaths[type], MAX_PATH_SIZE, "%s",
								   newLogFilePath) >= MAX_PATH_SIZE;
	free(newLogFilePath);

	return couldCreateFile;
}

int createDir() {
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

int createPath(char *newLogFilePath, log_t type) {
	if (dir == NULL) {
		dir = "./";
	}

	size_t bytesWritten = snprintf(newLogFilePath, MAX_PATH_SIZE, "%s", dir);

	if (bytesWritten >= MAX_PATH_SIZE) {
		fprintf(stderr, "[LOG] Error: failed attempting to create the path to "
						"open/create the log file\n");
		return FAILED;
	}

	const char *log =
		(type == ACCESS_LOG ? "access.log" :
							  (type == DEBUG_LOG ? "debug.log" : "error.log"));

	bytesWritten = snprintf(newLogFilePath + strlen(dir),
							MAX_PATH_SIZE - strlen(dir), "%s", log);

	if (bytesWritten >= (MAX_PATH_SIZE - strlen(dir))) {
		fprintf(stderr, "[LOG] Error: failed attempting to create the path to "
						"open/create the log file\n");
		return FAILED;
	}

	return OK;
}

int getTime(char *buffer) {
	time_t now = time(0);

	return (snprintf(buffer, MAX_TIME_SIZE, "%s", ctime(&now)) < MAX_TIME_SIZE ?
				OK :
				FAILED);
}

int writeTime(char **curr, const char *end) {
	char *time = calloc(MAX_TIME_SIZE, sizeof(char));

	if (getTime(time) == FAILED) {
		free(time);
		return FAILED;
	}

	int couldWriteTime = writeLogElemToLogEntry(curr, end, "[%s] ", time);
	free(time);
	return couldWriteTime;
}

char *determineCommunication(httpADT_t s, communication_t action) {
	if (action == REQ) {
		return (char *) ((getRequestLineBuffer(s))->data);
	}
	else { // RESP
		return (char *) ((getResponseLineBuffer(s))->data);
	}
}

int setFirstLine(httpADT_t s, communication_t action, char *firstLine) {
	int bytesWritten = snprintf(firstLine, MAX_FIRST_LINE_LENGTH, "%s",
								determineCommunication(s, action));

	if (bytesWritten >= MAX_FIRST_LINE_LENGTH) {
		fprintf(stderr,
				"[LOG] Error: failed attempting to get the first line in "
				"the request/response\n");
		return FAILED;
	}

	return OK;
}

int writeFirstLine(httpADT_t s, communication_t action, char **curr,
				   const char *end) {
	char *firstLine = calloc(MAX_FIRST_LINE_LENGTH, sizeof(char));

	if (setFirstLine(s, action, firstLine) == FAILED) {
		free(firstLine);
		return FAILED;
	}

	if (writeLogElemToLogEntry(curr, end, "'%s'\n", firstLine) == FAILED) {
		free(firstLine);
		return FAILED;
	}

	free(firstLine);
	return OK;
}

int getIpAddress(struct sockaddr_in *addr, char *buff) {
	int bytesWritten =
		snprintf(buff, MAX_ADDR_SIZE, "%s", inet_ntoa(addr->sin_addr));

	if (bytesWritten >= MAX_ADDR_SIZE) {
		fprintf(stderr, "[LOG] Error: failed attempting to get the client "
						"and host addresses\n");
		return FAILED;
	}

	return OK;
}

int setClientAndHost(httpADT_t s, communication_t action) {
	if (client == NULL) {
		client = malloc(MAX_ADDR_SIZE);
	}

	if (host == NULL) {
		host = malloc(MAX_ADDR_SIZE);
	}

	if (action == REQ) {
		if (getIpAddress((struct sockaddr_in *) getClientAddress(s), client) ==
			FAILED) {
			free(client);
			free(host);
			return FAILED;
		}

		struct sockaddr *originIP = getOriginResolutions(s)->ai_addr;

		if (getIpAddress((struct sockaddr_in *) originIP, host) == FAILED) {
			free(client);
			free(host);
			return FAILED;
		}
	}

	return OK;
}

int writeClientAndHost(char **curr, const char *end, char *client, char *host,
					   communication_t action) {
	if (action == RESP) {
		char *aux = client;
		client	= host;
		host	  = aux;
	}

	if (writeLogElemToLogEntry(curr, end, "[%s --> ", client) == FAILED) {
		return FAILED;
	}

	if (writeLogElemToLogEntry(curr, end, "%s] - ", host) == FAILED) {
		return FAILED;
	}

	return OK;
}

int writeCustomError(char *errorMsg, char **curr, const char *beginning) {
	if (errorMsg == NULL || (strcmp(errorMsg, "") == 0)) {
		errorMsg = "Error";
	}

	if (writeLogElemToLogEntry(curr, beginning + MAX_ENTRY_SIZE, "%s",
							   errorMsg) == FAILED) {
		return FAILED;
	}

	return OK;
}

int writeSystemError(char **curr, const char *beginning, logError_t errorType) {
	if (errorType == SYS_ERROR) {
		if (writeLogElemToLogEntry(curr, beginning + MAX_ENTRY_SIZE, ": %s",
								   strerror(errno)) == FAILED) {
			return FAILED;
		}
	}

	if (writeLogElemToLogEntry(curr, beginning + MAX_ENTRY_SIZE, "%s\n", "") ==
		FAILED) {
		return FAILED;
	}

	return OK;
}

int writeLogElemToLogEntry(char **curr, const char *end, const char *format,
						   const char *data) {
	if (*curr == NULL) {
		fprintf(stderr,
				"[LOG] Error: failed attempting to create the log "
				"entry, could not reserve space (malloc) for the log entry "
				"variable\n");
		return FAILED;
	}

	int bytesWritten = snprintf(*curr, (end - (*curr)), format, data);

	if (bytesWritten >= (end - (*curr))) {
		fprintf(stderr, "[LOG] Error: failed attempting to create the log "
						"entry, the buffer ran out of space for the string\n");
		return FAILED;
	}

	*curr = ((*curr) + bytesWritten);

	return OK;
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

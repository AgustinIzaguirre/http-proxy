#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <logger.h>
#include <stdio.h>
#include <time.h>
#include <http.h>

#define MAX_ENTRY_SIZE 512
#define MAX_ADDR_SIZE 128
#define MAX_PATH_SIZE 256

enum accessLogElems {
  REMOTE_ADDR,
  TIME,
  REQUEST,
  STATUS,
  // TODO: BODY_BYTES_SENT,
  HTTP_REFERER,
  HTTP_USER_AGENT
};

char **logFiles = NULL;


char *getTime();
int getIpAddresses(struct sockaddr_storage *clientAddr, char *host,
    char *server);
char *writeLogElemToLogEntry(char *curr, const char *end,
  const char *format, const char *data);
char *createPath(char *dir);
int existsLogFile(log_t logType);
int existsLogFilesArray();
void createLogFilesArray();
void checkLogFile(log_t type);
int createDir(const char *dir);


int createLogFile(log_t type) {
  FILE *newLogFile     = NULL;
  char *newLogFilePath = NULL;
  char *dir            = "./logs/";

  if (createDir(dir) == FAILED) {
    return FAILED;
  }

  if ((newLogFilePath = createPath(dir)) == NULL) {
    perror("Failed creating the path");
    return FAILED;
  }

  if ((newLogFile = fopen(newLogFilePath, "a+")) == NULL) {
    perror("Failed creating the file");
    return FAILED;
  }

  logFiles[type] = newLogFilePath;

  return  OK;
}

char *createPath(char *dir) {
  char *newLogFilePath  = calloc(MAX_PATH_SIZE, sizeof(char *));
  const char *accessLog = "access.log";
  size_t bytesWritten   = 0;

  if (dir == NULL) {
    dir = "./";
  }

  bytesWritten = snprintf(newLogFilePath, MAX_PATH_SIZE, "%s", dir);

  if (bytesWritten >= MAX_PATH_SIZE) {
    return NULL;
  }

  bytesWritten = snprintf(newLogFilePath + strlen(dir),
    MAX_PATH_SIZE - strlen(dir), "%s", accessLog);

  if (bytesWritten >= (MAX_PATH_SIZE - strlen(dir))) {
    return NULL;
  }

  return newLogFilePath;
}

char *createLogEntry(httpADT_t s) {
  char *beginning   = malloc(MAX_ENTRY_SIZE);
  char *curr        = beginning;
  const char *end   = curr + MAX_ENTRY_SIZE;
  char *host        = malloc(MAX_ADDR_SIZE);
  char *server      = malloc(MAX_ADDR_SIZE);
  char *requestLine = (char *)((getRequestLineBuffer(s))->data);

  if (getIpAddresses(getClientAddress(s), host, server) != 0) {
    return FAILED;
  }

  if ((curr = writeLogElemToLogEntry(curr, end, "[%s] - ", getTime())) == NULL) {
    return FAILED;
  }

  if ((curr = writeLogElemToLogEntry(curr, end, "%s ", host)) == NULL) {
    return FAILED;
  }

  if ((curr = writeLogElemToLogEntry(curr, end, "''%s'", requestLine)) == NULL) {
    return FAILED;
  }

  free(host);
  free(server);

  return curr < end ? beginning : NULL;
}

int addEntryToLog(httpADT_t http, log_t type) {
  char *logEntry = NULL;
  int couldWriteToFile = 0;
  FILE *logFile        = NULL;

  checkLogFile(type);
  logEntry = createLogEntry(http);

  if (logEntry == NULL) {
    return FAILED;
  }

  logFile = fopen(logFiles[type], "a+");
  couldWriteToFile = (fprintf(logFile, "%s", logEntry) >= 0);
  fclose(logFile);

  return couldWriteToFile;
}

int existsLogFile(log_t type) {
  return logFiles[type] != NULL;
}

int existsLogFilesArray() {
  return logFiles != NULL;
}

void createLogFilesArray() {
  logFiles = malloc(LOG_TYPES * sizeof(char*));

  for (int i = 0; i < LOG_TYPES; i++) {
    logFiles[i] = NULL;
  }
}

void checkLogFile(log_t type) {
  if (!existsLogFilesArray()) {
    createLogFilesArray();
  }

  if (!existsLogFile(type)) {
    createLogFile(type);
  }
}

int getIpAddresses(struct sockaddr_storage *clientAddr, char *host,
    char *server) {
  return getnameinfo((struct sockaddr *)clientAddr, sizeof(struct sockaddr), host,
    MAX_ADDR_SIZE, server, MAX_ADDR_SIZE, NI_NAMEREQD);
}

char *writeLogElemToLogEntry(char *curr, const char *end,
  const char *format, const char *data) {
  int bytesWritten = snprintf(curr, end - curr, format, data);

  if (bytesWritten >= (end - curr)) {
    return NULL;
  }

  curr += bytesWritten;

  return curr;
}

char *getTime() {
  time_t now = time(0);
  return ctime(&now);
}

int createDir(const char *dir) {
  struct stat st = {0};

  if (stat(dir, &st) == -1) {
    perror("Directory inexistent");

    if (mkdir(dir, 0700) == -1) {
      perror("Failed creating directory");
      return FAILED;
    }
  }

  return OK;
}

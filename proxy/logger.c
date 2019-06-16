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

char **logFiles = NULL;
char *client    = NULL;
char *host      = NULL;

char *getTime();
int getIpAddresses(struct sockaddr_storage *clientAddr, char *client,
    char *host);
char *writeLogElemToLogEntry(char *curr, const char *end,
  const char *format, const char *data);
char *createPath(char *dir);
int existsLogFile(log_t logType);
int existsLogFilesArray();
void createLogFilesArray();
void checkLogFile(log_t type);
int createDir(const char *dir);
char *determineCommunication(httpADT_t s, communication_t action);
char *writeClientAndHost(char *curr, const char *end, char *client,
  char *host, communication_t action);
int setClientAndHost(httpADT_t s, communication_t action);


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

char *createLogEntry(httpADT_t s, communication_t action) {
  char *beginning   = malloc(MAX_ENTRY_SIZE);
  char *curr        = beginning;
  const char *end   = curr + MAX_ENTRY_SIZE;
  char *firstLine   = determineCommunication(s, action);

  setClientAndHost(s, action);

  if ((curr = writeLogElemToLogEntry(curr, end, "[%s] ", getTime())) == NULL) {
    return FAILED;
  }

  if ((curr = writeClientAndHost(curr, end, client, host, action)) == NULL) {
    return FAILED;
  }

  if ((curr = writeLogElemToLogEntry(curr, end, "'%s'\n", firstLine)) == NULL) {
    return FAILED;
  }

  return curr < end ? beginning : NULL;
}

int addEntryToLog(httpADT_t http, log_t type, communication_t action) {
  char *logEntry       = NULL;
  int couldWriteToFile = 0;
  FILE *logFile        = NULL;

  checkLogFile(type);
  logEntry = createLogEntry(http, action);

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

int getIpAddresses(struct sockaddr_storage *clientAddr, char *client,
    char *host) {
  return getnameinfo((struct sockaddr *)clientAddr, sizeof(struct sockaddr), client,
    MAX_ADDR_SIZE, host, MAX_ADDR_SIZE, NI_NAMEREQD);
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
  char *time = calloc(128, sizeof(char));
  snprintf(time, 128, "%s", ctime(&now));
  size_t len = strlen(time);
  time[len - strlen("\n")] = 0;
  return time;
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

char *determineCommunication(httpADT_t s, communication_t action) {
  char *ret       = malloc(MAX_FIRST_LINE_LENGTH);
  char *firstLine = NULL;
  size_t len      = 0;

  if (action == REQ) {
    firstLine = (char *)((getRequestLineBuffer(s))->data);
  }
  else {
    firstLine = (char *)((getResponseLineBuffer(s))->data);
  }

  snprintf(ret, MAX_FIRST_LINE_LENGTH, "%s", firstLine);
  len                     = strlen(ret);
  ret[len - strlen("\n")] = 0;

  return ret;
}

char *writeClientAndHost(char *curr, const char *end, char *client,
  char *host, communication_t action) {
  char *aux = NULL;

  if (action == RESP) {
    aux    = client;
    client = host;
    host   = aux;
  }

  if ((curr = writeLogElemToLogEntry(curr, end, "[%s --> ", client)) == NULL) {
    return NULL;
  }

  if ((curr = writeLogElemToLogEntry(curr, end, "%s] - ", host)) == NULL) {
    return NULL;
  }

  return curr;
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

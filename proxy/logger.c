#include <logger.h>
#include <stdio.h>
/* TODO: Check the reason for gcc showing a warning telling me to include
 * <stdio.h> for the usage of snprintf when I have already included the library
 */
#include <time.h>
#include <http.h>

#define MAX_ENTRY_SIZE 512
#define MAX_ADDR_SIZE 128

enum accessLogElems {
  REMOTE_ADDR,
  TIME,
  REQUEST,
  STATUS,
  // TODO: BODY_BYTES_SENT,
  HTTP_REFERER,
  HTTP_USER_AGENT
};

char *getTime();
int getIpAddresses(struct sockaddr_storage *clientAddr, char *host,
    char *server);
int writeLogElemToLogEntry(char **beginning, const char *end,
  const char *format, const char *data);
char *createPath(char *newLogFilePath, size_t newLogFilePathSize,
  const char *path);

int createLogFile(const char *path) {
  // TODO: Check if size makes sense & if it should be set as #define
  size_t size = 64;
  char *newLogFilePath = malloc(size);

  createPath(newLogFilePath, size, path);

  if (newLogFilePath == NULL) {
    return FAILED;
  }

  return fopen(newLogFilePath, "+a") == NULL ? FAILED : OK;
}

char *createPath(char *newLogFilePath, size_t newLogFilePathSize,
  const char *path) {
  const char *accessLog = "access.log";
  int bytesWritten = 0;

  if (path == NULL) {
    path = "./";
  }

  bytesWritten = snprintf(newLogFilePath, newLogFilePathSize, "%s", path);

  if (bytesWritten >= sizeof newLogFilePath) {
    return NULL;
  }

  bytesWritten = snprintf(newLogFilePath + sizeof(path),
    sizeof(newLogFilePath) - sizeof(path), "%s", accessLog);

  if (bytesWritten >= (sizeof(newLogFilePath) - sizeof(path))) {
    return NULL;
  }

  return newLogFilePath;
}

char *createLogEntry(httpADT_t s) {
  char *curr = malloc(MAX_ENTRY_SIZE);
  const char *end = curr + MAX_ENTRY_SIZE;
  char *host = malloc(MAX_ADDR_SIZE);
  char *server = malloc(MAX_ADDR_SIZE);
  char *requestLine = (char *)((getRequestLineBuffer(s))->data);

  if (getIpAddresses(getClientAddress(s), host, server) == FAILED) {
    return FAILED;
  }

  if (writeLogElemToLogEntry(&curr, end, "[%s] ", getTime()) == FAILED) {
    return FAILED;
  }

  if (writeLogElemToLogEntry(&curr, end, "%s - ", host) == FAILED) {
    return FAILED;
  }

  if (writeLogElemToLogEntry(&curr, end, "\"%s\" ", requestLine) == FAILED) {
    return FAILED;
  }

  return curr < end ? curr : NULL;
}

int addEntryToLog(httpADT_t http, const char *logFileName) {
  // TODO: Check if size makes sense & if it should be set as #define's
  char *logEntryBuffer = malloc(MAX_ENTRY_SIZE);
  FILE *logFile;
  int couldWriteToFile = 0;

  logEntryBuffer = createLogEntry(http);
  if (logEntryBuffer == NULL) {
    return FAILED;
  }

  logFile = fopen (logFileName, "a+");
  couldWriteToFile = (fprintf(logFile, "%s", logEntryBuffer) >= 0);
  fclose(logFile);

  return couldWriteToFile;
}

/*
// TODO: Receive the response status saved in the proxy
int createLogEntryStr(struct sockaddr_storage *clientAddr,
    char *logEntryBuffer) {
  size_t maxAddrSize = 512;
  size_t maxRequestSize = 1024;
  char host[maxAddrSize], server[maxAddrSize];
  // TODO: Get the request from the first line saved in the proxy
  char request[maxRequestSize]; // TODO: Realloc request size if needed
  int bytesSupposedToWrite = 0;
  int couldGetIpAddresses = getIpAddresses(clientAddr, host, server);
  int ret = 0;

  if (couldGetIpAddresses != 0) {
    return FAILED;
  }

  return ret;
}
*/

int getIpAddresses(struct sockaddr_storage *clientAddr, char *host,
    char *server) {
  return getnameinfo((struct sockaddr *)clientAddr, sizeof(clientAddr), host,
    sizeof(host), server, sizeof(server), NI_NAMEREQD);
}

int writeLogElemToLogEntry(char **beginning, const char *end,
  const char *format, const char *data) {
  char *curr = *beginning;
  int bytesWritten = snprintf(curr, end - curr, format, data);

  if (bytesWritten >= (end - curr)) {
    return FAILED;
  }

  curr += bytesWritten;
  *beginning = curr;

  return OK;
}

char *getTime() {
  time_t now = time(0);
  return ctime(&now);
}


// ======================== SETTERS =======================================
// TODO> Check if setters make sense (is the structure necessary at all?)
// void setRemoteAddr(char **accessLog, const char *remoteAddr) {
//   accessLog[REMOTE_ADDR] = remoteAddr;
// }
//
// void setTime(char **accessLog) {
//   time_t now = time(0);
//   accessLog[TIME] = ctime(&now);
// }
//
// void setRequest(char **accessLog, const char *request) {
//   accessLog[REQUEST] = request;
// }
//
// void setStatus(char **accessLog, const char *status) {
//   accessLog[STATUS] = status;
// }
//
// void setHttpReferer(char **accessLog, const char *httpReferer) {
//   accessLog[HTTP_REFERER] = httpReferer;
// }
//
// void setHttpUserAgent(char **accessLog, const char *httpUserAgent) {
//   accessLog[HTTP_USER_AGENT] = httpUserAgent;
// }

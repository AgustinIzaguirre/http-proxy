#include <logger.h>
#include <stdio.h>
/* TODO: Check the reason for gcc showing a warning telling me to include
 * <stdio.h> for the usage of snprintf when I have already included the library
 */
#include <time.h>
#include <http.h>
#include <httpProxyADT.h>

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

char *createPath(char *newLogFilePath, char *path) {
  if (path == NULL) {
    path = "./";
  }

  bytesWritten = snprintf(newLogFilePath, sizeof newLogFilePath, "%s", path);

  if (bytesWritten >= sizeof newLogFilePath) {
    return NULL;
  }

  bytesWritten = snprintf(newLogFilePath + sizeof(path),
    sizeof(newLogFilePath) - sizeof(path), "access.log");

  if (bytesWritten >= (sizeof(newLogFilePath) - sizeof(path))) {
    return NULL;
  }

  return newLogFilePath;
}

int createLogFile(char *path) {
  // TODO: Check if size makes sense & if it should be set as #define
  size_t size = 64;
  char *newLogFilePath = malloc(size);
  char *accessLog = "access.log";
  int bytesWritten = 0;

  newLogFilePath = createPath(newLogFilePath, path);

  if (newLogFilePath == NULL) {
    return FAILED;
  }

  return fopen(newLogFilePath, "+a") == NULL ? FAILED : OK;
}

char *createLogEntry(httpADT_t s) {
  char *curr = malloc(MAX_ENTRY_SIZE);
  const char *end = curr + MAX_ENTRY_SIZE;
  int bytesWritten = 0;
  size_t logElemsSize = 6 // TODO: Should be 7 with BODY_BYTES_SENT
  char **logElems = malloc(logElemsSize);
  char *host = malloc(MAX_ADDR_SIZE);
  char *server = malloc(MAX_ADDR_SIZE);
  buffer *requestLine = getRequestLineBuffer(s);

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

int writeLogElemToLogEntry(char **beginning, char *end, const char *format,
  const char *data) {
  char *curr = *beginning;
  int bytesWritten = snprintf(curr, end - curr, format, data);

  if (bytesWritten >= (end - curr)) {
    return FAILED;
  }

  curr += bytesWritten;
  *beginning = curr;

  return OK;
}


// ======================== SETTERS =======================================

void setRemoteAddr(char **accessLog, const char *remoteAddr) {
  accessLog[REMOTE_ADDR] = remoteAddr;
}

void setTime(char **accessLog) {
  time_t now = time(0);
  accessLog[TIME] = ctime(&now);
}

void setRequest(char **accessLog, const char *request) {
  accessLog[REQUEST] = request;
}

void setStatus(char **accessLog, const char *status) {
  accessLog[STATUS] = status;
}

void setHttpReferer(char **accessLog, const char *httpReferer) {
  accessLog[HTTP_REFERER] = httpReferer;
}

void setHttpUserAgent(char **accessLog, const char *httpUserAgent) {
  accessLog[HTTP_USER_AGENT] = httpUserAgent;
}


// ======================== GETTERS =======================================

char *getTime() {
  time_t now = time(0);
  return ctime(&now);
}

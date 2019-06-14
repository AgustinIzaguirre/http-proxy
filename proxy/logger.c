#include <logger.h>
#include <stdio.h>
/* TODO: Check the reason for gcc showing a warning telling me to include
 * <stdio.h> for the usage of snprintf when I have already included the library
 */
#include <time.h>
#include <http.h>
#include <httpProxyADT.h>

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

char *createLogEntry(const char **logElems, size_t logElemsSize, char *buff,
  size_t buffSize) {
  char const *end = buff + buffSize;
  int bytesWritten = 0;

  // TODO: Format correctly
  for (int i = 0; (i < logElemsSize) && (buff < end); i++) {
    bytesWritten = snprintf(buff, end - buff, "%s", logElems[i]);

    if (bytesWritten >= (end - buff))
      return FAILED;
    else
      buff += bytesWritten;
  }

  return buff < end ? buff : NULL;
}

int addEntryToLog(struct sockaddr_storage *clientAddr,
  const char *logFileName) {
  // TODO: Check if size makes sense & if it should be set as #define's
  size_t maxLogEntrySize = 512;
  char logEntryBuffer[maxLogEntrySize];
  FILE *logFile;
  int couldWriteToFile = 0;

  if (!createLogEntry(clientAddr, logEntryBuffer)) {
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

char **createLogElemsArray() {
  size_t logElemsAmount = 7; // TODO: Should be 8 with BODY_BYTES_SENT
  char **logElems = malloc(logElemsAmount);

  return logElems;
}

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

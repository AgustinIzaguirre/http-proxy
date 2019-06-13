#include <logger.h>
#include <stdio.h>
/* TODO: Check the reason for gcc showin a warning telling me to include
 * <stdio.h> for the usage of snprintf when I have already included the library
 */
#include <time.h>
#include <http.h>
#include <httpProxyADT.h>

void createLogFile(const char *path) {
  // TODO: Check if size makes sense & if it should be set as #define
  size_t size = 64;
  char newLogFilePath[size];
  char *accessLog = "access.log";
  int bytesSupposedToWrite = 0;

  if (path == NULL)
    strcpy(newLogFilePath, "./");
  else
    strcpy(newLogFilePath, path);

  bytesSupposedToWrite = snprintf(newLogFilePath + size, sizeof newLogFilePath,
      "%s%s", newLogFilePath, accessLog);

  fopen(newLogFilePath, "+a");
}

int addLogEntry(struct sockaddr_storage *clientAddr, const char *logFileName) {
  // TODO: Check if size makes sense & if it should be set as #define's
  size_t maxLogEntrySize = 512;
  char logEntryBuffer[maxLogEntrySize];
  FILE *logFile;
  int ret = 0;

  if (!createLogEntryStr(clientAddr, logEntryBuffer))
    return ERROR;

  logFile = fopen (logFileName, "a+");

  ret = fprintf(logFile, "%s", logEntryBuffer) <= 0;
  fclose(logFile);

  return ret;
}

// TODO: Receive the response status saved in the proxy
int createLogEntryStr(struct sockaddr_storage *clientAddr,
    char *logEntryBuffer) {
  // TODO: Check if sizes make sense & if they should be set as #define's
  size_t maxAddrSize = 512;
  size_t maxRequestSize = 1024;
  char host[maxAddrSize], server[maxAddrSize];
  // TODO: Get the request from the first line saved in the proxy
  char request[maxRequestSize]; // TODO: Realloc request size if needed
  int bytesSupposedToWrite = 0;
  int couldGetIpAddresses = getIpAddresses(clientAddr, host, server);
  int ret = 0;

  if (couldGetIpAddresses != 0)
    return ERROR;

  bytesSupposedToWrite = snprintf(logEntryBuffer, sizeof(logEntryBuffer), "%s",
    host);
  ret = bytesSupposedToWrite < sizeof(logEntryBuffer);

  return ret;
}

int getIpAddresses(struct sockaddr_storage *clientAddr, char *host,
    char *server) {
  return getnameinfo((struct sockaddr *)clientAddr, sizeof(clientAddr), host,
    sizeof(host), server, sizeof(server), NI_NAMEREQD);
}

char *getTime() {
  time_t now = time(0);
  return ctime(&now);
}

#include <logger.h>
#include <time.h>

void createLogFile(const char *path) {
  // TODO: Check if size makes sense & if it should be set as #define
  size_t size = 64;
  char newLogFilePath[size] = {0};
  char *accessLog = "access.log";

  if (path == NULL)
    strcpy(newLogFilePath, "./");
  else
    strcpy(newLogFilePath, path);

  snprintf(newLogFilePath + size, sizeof accessLog, "%s%s", newLogFilePath,
    accessLog);

  fopen(newLogFilePath, "+a");
}

int addLogEntry(httpADT_t http) {
  // TODO: Check if sizes make sense & if they should be set as #define's
  size_t maxAddrSize = 512, maxLogEntrySize = 512;
  char logEntry[maxLogEntrySize];
  char host[maxAddrSize];
  char server[maxAddrSize];
  int couldGetIpAddresses = getIpAddresses(http->clientAddr, host, server);

  if (couldGetIpAddresses != 0)
    ret = ERROR;

  snprintf(logEntry, sizeof(logEntry), "%s", host);
}

int getIpAddresses(sockaddr_storage clientAddr, char *host, char *server) {
  return getnameinfo((sockaddr *)clientAddr, sizeof(clientAddr), host,
    sizeof(host), server, sizeof(server), NI_NAMEREQD);
}

char * getTime() {
  time_t now = time(0);
  return localtime(&now);
  /* snprintf(buf, sizeof(buf), "[%d/%s/%d:%d%:d:%d %s]", day, month, year, hour,
    min, sec, ); */
}

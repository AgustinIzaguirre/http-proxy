#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_PROXY_HTTP_PORT 8080
#define DEFAULT_MANAGEMENT_PORT 9090
#define STDERR_REDIRECT_DEFAULT "/dev/null"
#define FALSE 0
#define TRUE 1
#define INVALID_FD -1

typedef struct configuration *configurationADT;

/* Returns configuration ADT */
configurationADT getConfiguration();

/*Initialize command stderr to /dev/null */
void initializeConfigBaseValues(configurationADT config);

/* Returns the fd where stderr should be redirected for the executed program */
int getCommandStderrFd(configurationADT config);

/* Sets the fd where stderr should be redirected for the executed program */
void setCommandStderrFd(configurationADT config, int errorFd);

/* Returs http proxy server port */
unsigned short getHttpPort(configurationADT config);

/* Sets http proxy server port to httpPort */
void setHttpPort(configurationADT config, unsigned short httpPort);

/* Returs http proxy management  port */
unsigned short getManagementPort(configurationADT config);

/* Returs http proxy management  port */
void setManagementPort(configurationADT config, unsigned short managementPort);

/* Returns proxy listening interfaces */
char *getHttpInterfaces(configurationADT config);

/* Sets proxy listening interfaces */
char *setHttpInterfaces(configurationADT config, char *httpInterfaces);

/* Returns management listening interfaces */
char *getManagementInterfaces(configurationADT config);

/* Sets management listening interfaces */
char *setManagementInterfaces(configurationADT config,
							  char *managementInterfaces);

#endif

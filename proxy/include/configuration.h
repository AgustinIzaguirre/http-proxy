#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utilities.h>
#include <mediaRange.h>

#define DEFAULT_PROXY_HTTP_PORT 8080
#define DEFAULT_MANAGEMENT_PORT 9090
#define STDERR_REDIRECT_DEFAULT "/dev/null"

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

/* Returns the path to the file where stderr should be redirected for the
 * executed program */
char *getCommandStderrPath(configurationADT config);

/* Sets the path to the file where stderr should be redirected for the executed
 * program */
void setCommandStderrPath(configurationADT config, char *errorPath);

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
void setHttpInterfaces(configurationADT config, char *httpInterfaces);

/* Returns management listening interfaces */
char *getManagementInterfaces(configurationADT config);

/* Sets management listening interfaces */
void setManagementInterfaces(configurationADT config,
							 char *managementInterfaces);

/* Sets transform command */
void setCommand(configurationADT config, char *command);

/* Returns transform command */
char *getCommand(configurationADT config);

/* Returns TRUE if transformations are enabled or FALSE otherwise */
uint8_t getIsTransformationOn(configurationADT config);

/* Set media range*/
void setMediaRange(configurationADT config, MediaRangePtr_t mediaRange);

/* Return media range*/
MediaRangePtr_t getMediaRange(configurationADT config);

/* Resets media range list, leaving it empty */
void resetMediaRangeList(configurationADT config);

/* Sets cmd and enable/disable transformation depending cmd length */
void setCommandAndTransformations(configurationADT config, char *command);

/* Sets transformation state to the given state */
void setTransformationState(configurationADT config, uint8_t state);

#endif

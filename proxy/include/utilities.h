#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdlib.h>

enum boolean { FALSE = 0, TRUE };

#define BLOCK 10

/*
 * Add a char to a dynamic string
 * Return the new address of the string and
 * adds 1 to the string size
 */
char *addCharToString(char *string, unsigned int *sizeString, char c);

#endif

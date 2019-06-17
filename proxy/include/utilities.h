#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdlib.h>
#include <stdint.h>
#include <buffer.h>

enum boolean { FALSE = 0, TRUE };

#define BLOCK 10

/*
 * Add a char to a dynamic string
 * Return the new address of the string and
 * adds 1 to the string size
 */
char *addCharToString(char *string, unsigned int *sizeString, char c);

/*
 * Receives a buffer and a int and write the int in the buffer as a string
 */
void writeNumber(buffer *chunkBuffer, int bytesRead);

/*
 * Returns the minimum ammount of digits needed to represent number in the
 * received base
 */
int getDigits(int number, int base);

/*
 * Given an hexa char return its value or -1 if it isn't an hexa value
 */
int getHexaValue(char l);

/*
 * Given an hexa string in data and the position of it last char returns its
 * decimal value
 */
unsigned long long hexaToULLong(char *data, int lastPosition);

#endif

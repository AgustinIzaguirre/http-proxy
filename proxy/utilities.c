#include <utilities.h>

char *addCharToString(char *string, unsigned int *sizeString, char c) {
	char *ret = string;
	if (*sizeString % BLOCK == 0) {
		ret = realloc(string, *sizeString + BLOCK);
	}
	ret[*sizeString] = c;
	(*sizeString)++;
	return ret;
}

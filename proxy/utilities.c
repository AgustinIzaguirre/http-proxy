#include <utilities.h>
#include <stdio.h>

char *addCharToString(char *string, unsigned int *sizeString, char c) {
	char *ret = string;
	if (*sizeString % BLOCK == 0) {
		ret = realloc(string, *sizeString + BLOCK);
	}
	ret[*sizeString] = c;
	(*sizeString)++;
	return ret;
}

inline int getDigits(int number, int base) {
	int digits	 = 0;
	int currNumber = number;
	while (currNumber) {
		currNumber /= base;
		digits++;
	}
	return digits;
}

void writeNumber(buffer *buffer, int number) {
	int size = getDigits(number, 16);
	char numberString[size];
	int i;
	sprintf(numberString, "%x", number);

	for (i = 0; i < size; i++) {
		buffer_write(buffer, numberString[i]);
	}
}
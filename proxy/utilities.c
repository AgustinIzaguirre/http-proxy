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

inline int getDigits(int number) {
	int digits	 = 0;
	int currNumber = number;
	while (currNumber) {
		currNumber /= 10;
		digits++;
	}
	return digits;
}

void writeNumber(buffer *chunkBuffer, int bytesRead) {
	int size = getDigits(bytesRead);
	char numberString[size];
	int i;
	sprintf(numberString, "%d", bytesRead);
	printf("number: %s\ndigits: %d\n", numberString, size); // TODO remove
	for (i = 0; i < bytesRead; i++) {
		buffer_write(chunkBuffer, numberString[i]);
	}
}
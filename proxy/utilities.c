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

	if (number == 0) {
		return 1;
	}

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

unsigned long long hexaToULLong(char *data, int lastPosition) {
	unsigned long long bytes = 0;
	unsigned long long pot   = 1;
	int position			 = lastPosition;
	while (position >= 0) {
		bytes += pot * getHexaValue(data[position]);
		position--;
		pot *= 16;
	}
	return bytes;
}

int getHexaValue(char l) {
	if (l >= '0' && l <= '9') {
		return l - '0';
	}
	else {
		int ret = -1;
		switch (l) {
			case 'A':
			case 'a':
				ret = 10;
				break;
			case 'b':
			case 'B':
				ret = 11;
				break;

			case 'C':
			case 'c':
				ret = 12;
				break;

			case 'D':
			case 'd':
				ret = 13;
				break;

			case 'E':
			case 'e':
				ret = 14;
				break;

			case 'F':
			case 'f':
				ret = 15;
				break;
		}
		return ret;
	}
}
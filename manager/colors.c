#include <colors.h>
#include <stdio.h>

void setPrintStyle(const char *attribute) {
	printf("%s", attribute);
}

void resetPrintStyle() {
	printf("%s", RESET);
}

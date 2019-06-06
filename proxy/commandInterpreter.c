#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <commandInterpreter.h>
#include <configuration.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void printfDefaultMessage(int option, char *value);
static unsigned stringToNumber(char *stringNumber);

int readOptions(const int argc, char *const *argv) {
	initializeConfigBaseValues(getConfiguration());
	int option;
	int i			   = 1;
	int params		   = 0;
	opterr			   = 0;
	char *validOptions = "e:hl:L:M:o:p:t:v";

	while (i < argc && (option = getopt(argc, argv, validOptions)) != -1) {
		switch (option) {
			case 'e':
				// redirects execv program stderr to fd
				printfDefaultMessage(option, optarg);
				params++;
				break;

			case 'h':
				printHelpMessage();
				break;

			case 'l':
				printfDefaultMessage(option, optarg);
				params++;
				break;

			case 'L':
				printfDefaultMessage(option, optarg);
				params++;
				break;

			case 'M':
				printfDefaultMessage(option, optarg);
				params++;
				break;

			case 'o':
				setManagementPort(getConfiguration(), stringToNumber(optarg));
				params++;
				break;
			case 'p':
				setHttpPort(getConfiguration(), stringToNumber(optarg));
				params++;
				break;

			case 't':
				setCommand(getConfiguration(), optarg);
				params++;
				break;

			case 'v':
				printVersion(option, NULL);
				break;

			case '?':
				if (NEEDS_ARGUMENT(optopt)) {
					fprintf(stderr, "Option -%c requires an argument.\n",
							optopt);
				}
				return -1;

			default:
				fprintf(stderr, "invalid arguments\n");
				return -1;
		}

		i++;
		params++;
	}

	// returns the quantity of parameters read
	return params;
}

void printfDefaultMessage(int option, char *value) {
	printf("%c value= %s\n\n", option, value == NULL ? "" : value);
}

void printHelpMessage() {
	printf("This is the help message, we should include\
		useful info here\n\n");
}

void printVersion() {
	printf("HTTPPROXY version 1.0.0 (Ubuntu 16.04.10)\n\n");
}

static unsigned stringToNumber(char *stringNumber) {
	unsigned number = stringNumber[0] - '0';
	int i			= 1;
	while (stringNumber[i]) {
		number = number * 10 + stringNumber[i] - '0';
		i++;
	}
	return number;
}
#ifndef COMMAND_INTERPRETER_H
#define COMMAND_INTERPRETER_H

#define NEEDS_ARGUMENT(option)                                                 \
	(((option) == 'e') || ((option) == 'l') || ((option) == 'L') ||            \
	 ((option) == 'm') || ((option) == 'o') || ((option) == 'p') ||            \
	 ((option) == 't'))

int readOptions(const int argc, char *const *argv);
void printHelpMessage();
void printVersion();

#endif

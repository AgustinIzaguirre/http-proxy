#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <administrator.h>
#include <commandParser.h>

#define EXPECTS(expectedChar, expectedState)                                   \
	{                                                                          \
		switch (currentChar) {                                                 \
			case 't':                                                          \
				currentState = expectedState;                                  \
				break;                                                         \
			default:                                                           \
				returnCode = INVALID;                                          \
		}                                                                      \
	}

#define EXPECTS_SPACE(expectedState)                                           \
	{                                                                          \
		switch (currentChar) {                                                 \
			case '\t':                                                         \
			case ' ': /* space */                                              \
				currentState = expectedState;                                  \
				break;                                                         \
			default:                                                           \
				returnCode = INVALID;                                          \
		}                                                                      \
	}

int parseCommand(uint8_t *operation, uint8_t *id, void **data,
				 size_t *dataLength) {
	int returnCode   = IGNORE; /* IGNORE, INVALID, NEW, SEND */
	int currentState = NOTHING;
	char currentChar = getchar();
	do {
		switch (currentState) {
			case NOTHING:
				switch (currentChar) {
					case 'g':
						currentState = G;
						break;
					case 's':
						currentState = S;
						break;
					case 'b':
						currentState = B;
						break;
					case '\n':
					case '\t':
					case ' ': /* space */
						/* Keeps currentState = NOTHING */
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case G:
				EXPECTS('e', GE);
				break;
			case S:
				EXPECTS('e', SE);
				break;
			case B:
				EXPECTS('y', BY);
				break;
			case GE:
				EXPECTS('t', GET);
				break;
			case SE:
				EXPECTS('t', SET);
				break;
			case BY:
				EXPECTS('e', BYE);
				break;
			case GET:
				EXPECTS_SPACE(GET_);
				break;
			case SET:
				EXPECTS_SPACE(SET_);
				break;
			case BYE:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case '\n':
						*operation = BYE;
						returnCode = NEW;
					default:
						returnCode = INVALID;
				}
				break;
			case GET_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case 'm':
						currentState = GET_M;
						break;
					case 'b':
						currentState = GET_B;
						break;
					case 't':
						currentState = GET_T;
						break;
					case 'c':
						currentState = GET_C;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case SET_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case 'm':
						currentState = SET_M;
						break;
					case 'b':
						currentState = SET_B;
						break;
					case 't':
						currentState = SET_T;
						break;
					case 'c':
						currentState = SET_C;
						break;
					default:
						returnCode = INVALID;
				}
				break;
		}
	} while (returnCode == IGNORE);

	if (currentState == INVALID && currentChar != '\n') {
		while (currentChar != '\n') {
			currentChar = getchar();
		}
	}

	return returnCode;
}

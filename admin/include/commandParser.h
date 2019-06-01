#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

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

#define EXPECTS_ENTER_ALLOWING_SPACES(block)                                   \
	{                                                                          \
		switch (currentChar) {                                                 \
			case '\t':                                                         \
			case ' ': /* space */                                              \
				/* Keeps current state */                                      \
				break;                                                         \
			case '\n':                                                         \
				block break;                                                   \
			default:                                                           \
				returnCode = INVALID;                                          \
		}                                                                      \
	}

int parseCommand(uint8_t *operation, uint8_t *id, void **data,
				 size_t *dataLength);

enum state_t {
	NOTHING,
	DOT,
	G,
	GE,
	GET,
	GET_,
	GET_M,
	GET_MT,
	GET_MTR,
	GET_MTR_,
	GET_MTR_B,
	GET_MTR_BT,
	GET_MTR_C,
	GET_MTR_CN,
	GET_MTR_H,
	GET_MTR_HS,
	B,
	BY,
	BYE,
	GET_B,
	GET_BF,
	GET_C,
	GET_CM,
	GET_CMD,
	GET_MI,
	GET_MIM,
	GET_MIME,
	GET_T,
	GET_TF,
	S,
	SE,
	SET,
	SET_,
	SET_M,
	SET_T,
	SET_B,
	SET_C
};

enum returnCode_t { IGNORE, INVALID, NEW, SEND };

#endif

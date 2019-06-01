#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <admin.h>

#define EXPECTS(expectedChar, expectedState)                                   \
	{                                                                          \
		switch (currentChar) {                                                 \
			case expectedChar:                                                 \
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
	B,
	BY,
	BYE,
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
	SET_MI,
	SET_MIM,
	SET_MIME,
	SET_T,
	SET_TF,
	SET_B,
	SET_BF,
	SET_C,
	SET_CM,
	SET_CMD
};

enum returnCode_t { IGNORE, INVALID, NEW, SEND };

#endif

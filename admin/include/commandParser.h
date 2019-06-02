#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <admin.h>
#include <ctype.h>

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
	BYE, /* command */
	G,
	GE,
	GET,
	GET_,
	GET_M,
	GET_MT,
	GET_MTR, /* command */
	GET_MTR_,
	GET_MTR_B,
	GET_MTR_BT, /* command */
	GET_MTR_C,
	GET_MTR_CN, /* command */
	GET_MTR_H,
	GET_MTR_HS, /* command */
	GET_B,
	GET_BF, /* command */
	GET_C,
	GET_CM,
	GET_CMD, /* command */
	GET_MI,
	GET_MIM,
	GET_MIME, /* command */
	GET_T,
	GET_TF, /* command */
	S,
	SE,
	SET,
	SET_,
	SET_M,
	SET_MI,
	SET_MIM,
	SET_MIME, /* command */
	SET_MIME_,
	SET_MIME_DATA, /* command */
	SET_T,
	SET_TF,
	SET_TF_,
	SET_TF_O,
	SET_TF_ON, /* command */
	SET_TF_OF,
	SET_TF_OFF, /* command */
	SET_B,
	SET_BF,
	SET_BF_,
	SET_BF_DATA, /* command */
	SET_BF_DATA_,
	SET_C,
	SET_CM,
	SET_CMD,
	SET_CMD_,
	SET_CMD_DATA, /* command */
	SET_CMD_DATA_
};

enum returnCode_t { IGNORE, INVALID, NEW, SEND };

#endif

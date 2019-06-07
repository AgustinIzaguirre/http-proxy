#include <commandParser.h>

int parseCommand(operation_t *operation, id_t *id, void **data,
				 size_t *dataLength) {
	returnCode_t returnCode = IGNORE;
	state_t currentState	= NOTHING;
	char currentChar;
	*data		= NULL;
	*dataLength = 0;
	*id			= NO_ID;

	// TODO: remember to use htons to transform to network-bytes

	do {
		currentChar = getchar();

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
					case '.':
						currentState = DOT;
						break;
					case '\n':
					case '\t':
					case ' ': /* space */
						/* Keeps currentState */
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
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *bye* */
					*operation = BYE_OP;
					returnCode = NEW;
				});
				break;
			case DOT:
				EXPECTS_ENTER_ALLOWING_SPACES({ returnCode = SEND; });
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
			case GET_M:
				switch (currentChar) {
					case 't':
						currentState = GET_MT;
						break;
					case 'i':
						currentState = GET_MI;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case GET_MT:
				EXPECTS('r', GET_MTR);
				break;
			case GET_MTR:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						currentState = GET_MTR_;
						break;
					case '\n':
						/* Set command information to *get mtr* */
						*operation = GET_OP;
						*id		   = MTR_ID;
						returnCode = NEW;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case GET_MTR_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case 'c':
						currentState = GET_MTR_C;
						break;
					case 'h':
						currentState = GET_MTR_H;
						break;
					case 'b':
						currentState = GET_MTR_B;
						break;
					case '\n':
						/* Set command information to *get mtr* */
						*operation = GET_OP;
						*id		   = MTR_ID;
						returnCode = NEW;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case GET_MTR_C:
				EXPECTS('n', GET_MTR_CN);
				break;
			case GET_MTR_H:
				EXPECTS('s', GET_MTR_HS);
				break;
			case GET_MTR_B:
				EXPECTS('t', GET_MTR_BT);
				break;
			case GET_MTR_CN:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get mtr cn* */
					*operation = GET_OP;
					*id		   = MTR_CN_ID;
					returnCode = NEW;
				});
				break;
			case GET_MTR_HS:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get mtr hs* */
					*operation = GET_OP;
					*id		   = MTR_HS_ID;
					returnCode = NEW;
				});
				break;
			case GET_MTR_BT:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get mtr bt* */
					*operation = GET_OP;
					*id		   = MTR_BT_ID;
					returnCode = NEW;
				});
				break;
			case GET_B:
				EXPECTS('f', GET_BF);
				break;
			case GET_BF:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get bf* */
					*operation = GET_OP;
					*id		   = BF_ID;
					returnCode = NEW;
				});
				break;
			case GET_C:
				EXPECTS('m', GET_CM);
				break;
			case GET_CM:
				EXPECTS('d', GET_CMD);
				break;
			case GET_CMD:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get cmd* */
					*operation = GET_OP;
					*id		   = CMD_ID;
					returnCode = NEW;
				});
				break;
			case GET_MI:
				EXPECTS('m', GET_MIM);
				break;
			case GET_MIM:
				EXPECTS('e', GET_MIME);
				break;
			case GET_MIME:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get mime* */
					*operation = GET_OP;
					*id		   = MIME_ID;
					returnCode = NEW;
				});
				break;
			case GET_T:
				EXPECTS('f', GET_TF);
				break;
			case GET_TF:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get tf* */
					*operation = GET_OP;
					*id		   = TF_ID;
					returnCode = NEW;
				});
				break;
			case SET_B:
				EXPECTS('f', SET_BF);
				break;
			case SET_BF:
				*dataLength = sizeof(uint32_t);
				*data		= calloc(1, *dataLength);
				EXPECTS_SPACE(SET_BF_);
				break;
			case SET_BF_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						**((uint32_t **) data) =
							**((uint32_t **) data) * 10 + currentChar - '0';
						currentState = SET_BF_DATA;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case SET_BF_DATA:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						currentState = SET_BF_DATA_;
						break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						**((uint32_t **) data) =
							**((uint32_t **) data) * 10 + currentChar - '0';
						currentState = SET_BF_DATA;
						break;
					case '\n':
						/* Set command information to *get bf int* */
						*operation = GET_OP;
						*id		   = BF_ID;
						returnCode = NEW;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case SET_BF_DATA_:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *get bf int* */
					*operation = GET_OP;
					*id		   = BF_ID;
					returnCode = NEW;
				});
				break;
			case SET_T:
				EXPECTS('f', SET_TF);
				break;
			case SET_TF:
				EXPECTS_SPACE(SET_TF_);
				break;
			case SET_TF_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case 'o':
						currentState = SET_TF_O;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case SET_TF_O:
				switch (currentChar) {
					case 'n':
						currentState = SET_TF_ON;
						break;
					case 'f':
						currentState = SET_TF_OF;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case SET_TF_ON:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *set tf on* */
					*operation			  = SET;
					*id					  = TF_ID;
					*dataLength			  = sizeof(uint8_t);
					*data				  = malloc(*dataLength);
					**((uint8_t **) data) = ON;
					returnCode			  = NEW;
				});
				break;
			case SET_TF_OF:
				EXPECTS('f', SET_TF_OFF);
				break;
			case SET_TF_OFF:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command information to *set tf off* */
					*operation			  = SET;
					*id					  = TF_ID;
					*dataLength			  = sizeof(uint8_t);
					*data				  = malloc(*dataLength);
					**((uint8_t **) data) = OFF;
					returnCode			  = NEW;
				});
				break;
			case SET_M:
				EXPECTS('i', SET_MI);
				break;
			case SET_MI:
				EXPECTS('m', SET_MIM);
				break;
			case SET_MIM:
				EXPECTS('e', SET_MIME);
				break;
			case SET_MIME:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						currentState = SET_MIME_;
						break;
					case '\n':
						/* Set command information to *set mime* */
						*operation = SET_OP;
						*id		   = MIME_ID;
						returnCode = NEW;
						break;
					default:
						returnCode = INVALID;
				}
				break;
			case SET_MIME_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					case '\n':
						/* Set command information to *set mime* */
						*operation = SET_OP;
						*id		   = MIME_ID;
						returnCode = NEW;
						break;
					default:
						if (isprint(currentChar)) {
							/* Start to allocate data */
							*data = malloc(PARSER_MALLOC_BLOCK);
							*(*((char **) data) + *dataLength) = currentChar;
							(*dataLength)++;
							currentState = SET_MIME_DATA;
						}
						else {
							returnCode = INVALID;
						}
				}
				break;
			case SET_MIME_DATA:
				switch (currentChar) {
					case '\n':
						/* Set command info *set mime media-type* */
						*operation = SET_OP;
						*id		   = MIME_ID;
						returnCode = NEW;
						break;
					default:
						if (isprint(currentChar)) {
							/* Continuing allocating data */
							if (*dataLength % PARSER_MALLOC_BLOCK == 0) {
								*data = realloc(
									*data,
									PARSER_MALLOC_BLOCK *
										(*dataLength / PARSER_MALLOC_BLOCK +
										 1));
							}
							*(*((char **) data) + *dataLength) = currentChar;
							(*dataLength)++;
							/* Keeps current state */
						}
						else {
							returnCode = INVALID;
						}
				}
				break;
			case SET_C:
				EXPECTS('m', SET_CM);
				break;
			case SET_CM:
				EXPECTS('d', SET_CMD);
				break;
			case SET_CMD:
				EXPECTS_SPACE(SET_CMD_);
				break;
			case SET_CMD_:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						/* Keeps current state */
						break;
					default:
						if (isprint(currentChar)) {
							/* Start to allocate data */
							*data = malloc(PARSER_MALLOC_BLOCK);
							*(*((char **) data) + *dataLength) = currentChar;
							(*dataLength)++;
							currentState = SET_CMD_DATA;
						}
						else {
							returnCode = INVALID;
						}
				}
				break;
			case SET_CMD_DATA:
				switch (currentChar) {
					case '\t':
					case ' ': /* space */
						currentState = SET_CMD_DATA_;
						break;
					case '\n':
						/* Set command info *set cmd command* */
						*operation = SET_OP;
						*id		   = CMD_ID;
						returnCode = NEW;
						break;
					default:
						if (isprint(currentChar)) {
							/* Continuing allocating/generating data */
							if (*dataLength % PARSER_MALLOC_BLOCK == 0) {
								*data = realloc(
									*data,
									PARSER_MALLOC_BLOCK *
										(*dataLength / PARSER_MALLOC_BLOCK +
										 1));
							}
							*(*((char **) data) + *dataLength) = currentChar;
							(*dataLength)++;
							/* Keeps current state */
						}
						else {
							returnCode = INVALID;
						}
				}
				break;
			case SET_CMD_DATA_:
				EXPECTS_ENTER_ALLOWING_SPACES({
					/* Set command info *set mime media-type* */
					*operation = SET_OP;
					*id		   = MIME_ID;
					returnCode = NEW;
				});
				break;
		}
	} while (returnCode == IGNORE);

	if (returnCode == INVALID) {
		if (currentChar != '\n') {
			while (currentChar != '\n') {
				currentChar = getchar();
			}
		}

		if (*data != NULL) {
			free(*data);
			*data = NULL;
		}

		*dataLength = 0;
	}

	return returnCode;
}

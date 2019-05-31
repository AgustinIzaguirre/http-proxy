#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

uint8_t parseCommand(uint8_t *operation, uint8_t *id, void **data,
					 size_t *dataLength);

#endif

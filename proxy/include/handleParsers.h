#ifndef HANDLE_PARSERS_H
#define HANDLE_PARSERS_H

#include <selector.h>
#include <httpProxyADT.h>
#include <stdio.h> //TODO: remove

#define GET_DATA(key) ((httpADT_t)(key)->data)

unsigned parseRead(struct selector_key *key);

void parseInit(const unsigned state, struct selector_key *key);
void parseDestroy(const unsigned state, struct selector_key *key);

#endif

#ifndef HANDLE_PARSERS_H
#define HANDLE_PARSERS_H

#include <selector.h>
#include <httpProxyADT.h>
#include <stdio.h> //TODO: remove

#define GET_DATA(key) ((httpADT_t)(key)->data)

unsigned parseRead(struct selector_key *key);

void parseRequestInit(const unsigned state, struct selector_key *key);
void parseRequestDestroy(const unsigned state, struct selector_key *key);

void parseTargetArrive(const unsigned state, struct selector_key *key);
void parseTargetDeparture(const unsigned state,
						  struct selector_key *key); // TODO

void parseVersionArrive(const unsigned state, struct selector_key *key);
void parseVersionDeparture(const unsigned state, struct selector_key *key);

void parseHeaderArrive(const unsigned state, struct selector_key *key);
void parseHeaderDeparture(const unsigned state, struct selector_key *key);

#endif

#ifndef HANDLE_REQUEST_H
#define HANDLE_REQUEST_H

#include <selector.h>

unsigned requestRead(struct selector_key *key);

unsigned requestWrite(struct selector_key *key);

#endif
#ifndef METRIC_H
#define METRIC_H

#include <stdint.h>

void increaseConcurrentConections();
void decreaseConcurrentConections();
uint64_t getConcurrentConections();

void increaseHistoricAccess();
uint64_t getHistoricAccess();

void increaseTransferBytes();
uint64_t getTransferBytes();

#endif

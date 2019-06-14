#ifndef METRIC_H
#define METRIC_H

#include <stdint.h>

/*
 * Increase by one the number of ConcurrentConections
 */
void increaseConcurrentConections();

/*
 * Decrease by one the number of ConcurrentConections
 */
void decreaseConcurrentConections();

/*
 * Returns the value of ConcurrentConection
 */
uint64_t getConcurrentConections();

/*
 * Increase by one the number of HistoricAccess
 */
void increaseHistoricAccess();

/*
 * Returns the value of HistoricAccess
 */
uint64_t getHistoricAccess();

/*
 * Increase by n the number of TransferBytes
 */
void increaseTransferBytes(uint64_t n);

/*
 * Returns the value of TransferBytes
 */
uint64_t getTransferBytes();

#endif

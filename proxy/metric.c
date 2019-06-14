#include <metric.h>

struct metrics {
	uint64_t concurrentConections;
	uint64_t historicAccess;
	uint64_t transferBytes;
};

static struct metrics metricSingleton = {
	.concurrentConections = 0,
	.historicAccess		  = 0,
	.transferBytes		  = 0,
};

void increaseConcurrentConections() {
	metricSingleton.concurrentConections++;
}

void decreaseConcurrentConections() {
	metricSingleton.concurrentConections--;
}

void increaseHistoricAccess() {
	metricSingleton.historicAccess++;
}

void increaseTransferBytes(uint64_t n) {
	metricSingleton.transferBytes += n;
}

uint64_t getConcurrentConections() {
	return metricSingleton.concurrentConections;
}

uint64_t getHistoricAccess() {
	return metricSingleton.historicAccess;
}

uint64_t getTransferBytes() {
	return metricSingleton.transferBytes;
}

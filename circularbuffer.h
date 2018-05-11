#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

typedef struct {
	void* buffer;
	uint8_t bufSize;
	size_t elemSize;
	bool* isNew;
	uint8_t nextIdx;	// index of next packet to act on
	uint8_t addIdx;		// index of next location to add a new packet
} CircularBuffer_ts;

CircularBuffer_ts*
circularbuffer_init(CircularBuffer_ts*, uint8_t, size_t);

void
circularbuffer_free(CircularBuffer_ts*);

bool
circularbuffer_add(CircularBuffer_ts*, void*, void (*)(void*, void*));

void*
circularbuffer_getNext(CircularBuffer_ts*);

#endif

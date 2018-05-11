#ifndef WINDOW_BUFFER_H
#define WINDOW_BUFFER_H

#include <stdbool.h>
#include <inttypes.h>

typedef struct {
	void* buffer;
	uint16_t* timestamp;
	bool* acknowledged;
	uint8_t bufSize;
	size_t elementSize;
	uint8_t windowSize;
	uint8_t lastAckRec;
	uint8_t lastMsgSent;
	uint8_t windowIdx;
	uint16_t timeout;
} WindowBuffer_ts;

WindowBuffer_ts*
windowbuffer_init(WindowBuffer_ts*, uint8_t, size_t, uint8_t, uint16_t);

void
windowbuffer_free(WindowBuffer_ts*);

void*
windowbuffer_addElement(WindowBuffer_ts*, void*, void (*)(void*, void*));

void
windowbuffer_print(WindowBuffer_ts*, void (*)(void*));

uint8_t
windowbuffer_sendReadyElements(WindowBuffer_ts*, uint8_t [], size_t, uint16_t );

void
windowbuffer_acknowledge(WindowBuffer_ts*, uint8_t);

#endif

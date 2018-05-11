#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "node.h"
#include "windowbuffer.h"

#define INC(i) MOD_INC(i, wBuffer->bufSize)
#define PLUS1(i) MOD_PLUS1(i, wBuffer->bufSize)

#define TIMEOUT_TIME wBuffer->timeout
#define TIMEOUT(ts, now) (((now) - (ts)) > TIMEOUT_TIME)

/**
 * Initializes the window buffer
 * All messages in buffer are initially acknowledged.  Acknowledged messages may
 * be overwritten by new messages.
 */
WindowBuffer_ts*
windowbuffer_init(WindowBuffer_ts* wBuffer, uint8_t bufferSize, size_t sz,
uint8_t wdwSz, uint16_t tmOut) {
	/* Allocate space for the arrays in the wBuffer */
	wBuffer->buffer = calloc(bufferSize, sz);
	wBuffer->timestamp = (uint16_t*)calloc(bufferSize, sizeof(uint16_t));
	wBuffer->acknowledged = (bool*)calloc(bufferSize, sizeof(bool));

	/* Initialize wBuffer values */
	wBuffer->bufSize = bufferSize;
	wBuffer->elementSize = sz;
	wBuffer->windowIdx = wBuffer->windowSize = wdwSz;
	wBuffer->lastMsgSent = wBuffer->lastAckRec = 0;
	wBuffer->timeout = tmOut;

	/* Initialize all elements to acknowledged */
	memset(wBuffer->acknowledged, true, wBuffer->bufSize);

	/* If any allocations failed return NULL, otherwise return the wBuffer */
	return wBuffer->buffer != NULL &&
		wBuffer->timestamp != NULL &&
		wBuffer->acknowledged != NULL ? wBuffer : NULL;
}

void windowbuffer_free(WindowBuffer_ts* wBuffer) {
	free(wBuffer->buffer);
	free(wBuffer->timestamp);
	free(wBuffer->acknowledged);
}

void*
windowbuffer_addElement(WindowBuffer_ts* wBuffer, void* el,
	void (*copy)(void*, void*)) {

	/* Find next available element */
	uint8_t i;
	for(i = PLUS1(wBuffer->lastMsgSent); i != wBuffer->lastAckRec; INC(i))
		if(wBuffer->acknowledged[i])
			break;

	/* If no available elements return NULL */
	if(i == wBuffer->lastAckRec)
		return NULL;

	/* Copy element into the wBuffer */
	void* dest = wBuffer->buffer + i * wBuffer->elementSize;
	copy(dest, el);
	wBuffer->acknowledged[i] = false;

	return dest;
}

void windowbuffer_print(WindowBuffer_ts* wBuffer, void (*printElem)(void*)) {
	for(uint8_t i = PLUS1(wBuffer->lastAckRec); i != wBuffer->lastAckRec;
	INC(i))
		if(!wBuffer->acknowledged[i])
			printElem(wBuffer->buffer + i * wBuffer->elementSize);
}

uint8_t
windowbuffer_sendReadyElements(WindowBuffer_ts* wBuffer, uint8_t indices[],
size_t n, uint16_t now) {
	uint8_t nSent = 0;

	/* Add timed out messages to the transmission window */
	for(uint8_t i = PLUS1(wBuffer->lastAckRec);
	i != PLUS1(wBuffer->lastMsgSent); INC(i)) {
		if(!wBuffer->acknowledged[i] && TIMEOUT(wBuffer->timestamp[i], now) &&
		nSent < n) {
//			printf("Resending message\n");
			wBuffer->timestamp[i] = now;
			indices[nSent++] = i;
		}
	}

	/* Add new messages to the transmission window */
	for(uint8_t i = PLUS1(wBuffer->lastMsgSent); i != PLUS1(wBuffer->windowIdx);
	INC(i)) {
		if(!wBuffer->acknowledged[i] &&	nSent < n) {
//			printf("Sending new message\n");
			wBuffer->timestamp[i] = now;
			indices[nSent++] = i;
			INC(wBuffer->lastMsgSent);
		}
	}

	return nSent;
}

void
windowbuffer_acknowledge(WindowBuffer_ts* wBuffer, uint8_t idx) {
	wBuffer->acknowledged[idx] = true;
	while(wBuffer->acknowledged[PLUS1(wBuffer->lastAckRec)])
		INC(wBuffer->lastAckRec);

	wBuffer->windowIdx = wBuffer->lastAckRec + wBuffer->windowSize;
}

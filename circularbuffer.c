#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "node.h"
#include "circularbuffer.h"

#define INC(i) MOD_INC(i, cBuffer->bufSize)

CircularBuffer_ts*
circularbuffer_init(CircularBuffer_ts* cBuffer, uint8_t nElem, size_t elemSz) {
	cBuffer->buffer = calloc(nElem, elemSz);
	cBuffer->bufSize = nElem;
	cBuffer->elemSize = elemSz;

	cBuffer->isNew = (bool*)calloc(nElem, sizeof(bool));

	cBuffer->nextIdx = cBuffer->addIdx = 0;

	return cBuffer->buffer != NULL && cBuffer->isNew != NULL ? cBuffer : NULL;
}

void
circularbuffer_free(CircularBuffer_ts* cBuffer) {
	free(cBuffer->buffer);
	free(cBuffer->isNew);
	cBuffer->bufSize = cBuffer->elemSize = 0;
	cBuffer->nextIdx = cBuffer->addIdx = 0;
}

bool
circularbuffer_add(CircularBuffer_ts* cBuffer, void* elem,
void (*copy)(void*, void*)) {
	bool canCopy = !cBuffer->isNew[cBuffer->addIdx];
	if(canCopy) {
		copy(cBuffer->buffer + cBuffer->addIdx * cBuffer->elemSize, elem);
		cBuffer->isNew[cBuffer->addIdx] = true;
		INC(cBuffer->addIdx);
	}

	return canCopy;
}

void*
circularbuffer_getNext(CircularBuffer_ts* cBuffer) {
	void* ret_val = NULL;

	if(cBuffer->isNew[cBuffer->nextIdx]) {
		cBuffer->isNew[cBuffer->nextIdx] = false;
		ret_val = cBuffer->buffer + cBuffer->nextIdx * cBuffer->elemSize;
		INC(cBuffer->nextIdx);
	}

	return ret_val;
}

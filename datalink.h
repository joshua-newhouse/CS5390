/**
 * dll.h
 * Author: Joshua Newhouse 15 April 2018
 */
#ifndef DATALINK_H
#define DATALINK_H

#include <stdbool.h>
#include <inttypes.h>

#include "node.h"
#include "circularbuffer.h"

typedef enum channelType {
	OBND_CHANNEL,
	IBND_CHANNEL
} ChannelType_te;

typedef enum {
	DL_LBL = 0,
	DL_CHL = 4,
	DL_SEQ = 5,
	DL_DATA = 7
} DatalinkHeader_te;

typedef struct {
	uint8_t byte[DL_FRM_SIZE];
} DatalinkFrame_ts;

bool datalink_init(void);
void datalink_terminate(void);
void datalink_receiveFromNetwork(CircularBuffer_ts*);
void datalink_receiveFromChannel(void);

#endif

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdbool.h>
#include <inttypes.h>

#include "node.h"
#include "network.h"

typedef enum {
	TRANS_MSG_TYPE = 0,
	TRANS_SRCID = 1,
	TRANS_DSTID = 2,
	TRANS_SEQNUM = 3,
	TRANS_DATA = 5
} TransportHeader_te;

typedef struct Message {
	uint8_t byte[TRANS_MSG_SIZE];
} TransportMessage_ts;

typedef struct Packet NetworkPacket_ts;

bool transport_init(char*);
void transport_sendString(void);
uint8_t transport_messageOffset(TransportMessage_ts*);
void transport_printMessage(void*);
void transport_terminate(void);
void transport_receiveFromNetwork(NetworkPacket_ts*);
void transport_writeMessages(void);

#endif

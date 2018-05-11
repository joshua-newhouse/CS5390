#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <inttypes.h>

#include "node.h"
#include "transport.h"
#include "circularbuffer.h"

typedef enum {
	NW_SRCID = 0,
	NW_DSTID = 1,
	NW_MSGID = 2,
	NW_DOFFS = 4,
	NW_DATA = 5
} NetworkHeader_te;

typedef struct Packet {
	uint8_t byte[NW_PKT_SIZE];
} NetworkPacket_ts;

typedef struct Message TransportMessage_ts;

bool network_init(void);
void network_terminate(void);
void network_receiveFromTransport(TransportMessage_ts*, uint8_t*, int);
void network_printPacket(NetworkPacket_ts*);
void network_receiveFromDatalink(CircularBuffer_ts*);
void network_addMessageToBuffer(TransportMessage_ts*);

#endif

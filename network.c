#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "node.h"
#include "circularbuffer.h"
#include "network.h"
#include "datalink.h"
#include "transport.h"

#define MAX_PACKETS 30

#define INC(i) MOD_INC(i, MAX_PACKETS)
#define PLUS1(i) MOD_PLUS1(i, MAX_PACKETS)

static uint8_t messageId;

/************************ Local Function Declarations *************************/

static void printPacket(NetworkPacket_ts*);
static void setPacketMsgId(uint8_t []);
static bool msgIdIsGreater(uint8_t [], uint8_t []);

/************************* Circular Buffer Definition *************************/
static CircularBuffer_ts sendBuffer;

/************************** Routing Table Definition **************************/
typedef struct {
	uint8_t highestMsgId[2];
} RoutingRecord_ts;

static RoutingRecord_ts routingTable[MAX_NODES];

/**************************** Function Definitions ****************************/

static void setPacketMsgId(uint8_t msgId[]) {
	msgId[0] = TO_ASCII(messageId / 10);
	msgId[1] = TO_ASCII(messageId % 10);

	MOD_INC(messageId, MAX_MSG_ID);
}

static void
packetCopy(void* dst, void* src) {
	memcpy(dst, src, sizeof(NetworkPacket_ts));
}

/**
 * Adds a transport layer message to the circular buffer as a network layer
 * packet.
 */
void
network_addMessageToBuffer(TransportMessage_ts* msg) {
	NetworkPacket_ts packet = {0};

	/* Fill packet with the data */
	packet.byte[NW_SRCID] = msg->byte[TRANS_SRCID];
	packet.byte[NW_DSTID] = msg->byte[TRANS_DSTID];
	setPacketMsgId(packet.byte + NW_MSGID);
	packet.byte[NW_DOFFS] = transport_messageOffset(msg);
	memcpy(packet.byte + NW_DATA, msg, sizeof(TransportMessage_ts));

//	network_printPacket(&packet);

	/* Sending new packets from this node so set the highest from this node */
	routingTable[SRC_ID].highestMsgId[0] = packet.byte[NW_MSGID];
	routingTable[SRC_ID].highestMsgId[1] = packet.byte[NW_MSGID + 1];

	circularbuffer_add(&sendBuffer, &packet, packetCopy);
}

/**
 * Prints a packet to stdout.
 */
void network_printPacket(NetworkPacket_ts* packet) {
	if(packet != NULL) {
		printf("PACKET:\n\tsrcId: %c destId: %c msgId: %c%c offset: %c\n",
			packet->byte[NW_SRCID], packet->byte[NW_DSTID],
			packet->byte[NW_MSGID], packet->byte[NW_MSGID + 1],
			packet->byte[NW_DOFFS]
		);
		transport_printMessage(packet->byte + NW_DATA);
	}
}

/**
 * Initilizes the routing table entries.
 */
bool network_init(void) {
	void* s;
	s = circularbuffer_init(&sendBuffer, MAX_PACKETS, sizeof(NetworkPacket_ts));

	for(int i = 0; i < MAX_NODES; i++) {
		routingTable[i].highestMsgId[0] = '0';
		routingTable[i].highestMsgId[1] = '0' - 1;
	}

	return s != NULL;
}

void network_terminate(void) {
	circularbuffer_free(&sendBuffer);
}

/**
 * Compares second message ID with first and returns true if second is greater
 * than first, false otherwise.
 */
static bool msgIdIsGreater(uint8_t second[], uint8_t first[]) {
	return (second[0] > first[0]) ||
		(second[0] == first[0] && second[1] > first[1]);
}

/**
 * network_receiveFromTransport - passes packets that are ready to be sent to 
 * datalink layer.
 */
void
network_receiveFromTransport(TransportMessage_ts* msg, uint8_t* trans, int n) {
//	printf("RECEIVING FROM TRANSPORT LAYER:\n");
	for(int i = 0; i < n; i++)
		network_addMessageToBuffer(&msg[trans[i]]);

	datalink_receiveFromNetwork(&sendBuffer);
}

void
network_receiveFromDatalink(CircularBuffer_ts* frameBuffer) {
	DatalinkFrame_ts* frame;

	while((frame = circularbuffer_getNext(frameBuffer)) != NULL) {
		NetworkPacket_ts packet = {0};
		memcpy(&packet, frame->byte + DL_DATA, sizeof(NetworkPacket_ts));

		/* Check routing table for the source.  If this message has a greater
		   msgId than previous for the source then process otherwise nothing. */
		uint8_t src = FROM_ASCII(packet.byte[NW_SRCID]);
		if(msgIdIsGreater(
		packet.byte + NW_MSGID, routingTable[src].highestMsgId)) {
			routingTable[src].highestMsgId[0] = packet.byte[NW_MSGID];
			routingTable[src].highestMsgId[1] = packet.byte[NW_MSGID + 1];

			/* If this packet is destined for this node then send to transport
			   else add it to the send buffer */
			if(packet.byte[NW_DSTID] == TO_ASCII(SRC_ID)) {
//				printf("NETWORK SENDING TO TRANSPORT: %d\n", SRC_ID);
				transport_receiveFromNetwork(&packet);
			}
			else {
//				printf("NETWORK FORWARDING: %d\n", SRC_ID);
//				network_printPacket(&packet);
				circularbuffer_add(&sendBuffer, &packet, packetCopy);
			}
		}
	}	
}

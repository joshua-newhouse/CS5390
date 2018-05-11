#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "node.h"
#include "windowbuffer.h"
#include "transport.h"
#include "network.h"

/* Transport Layer Constants and Macros */
#define TRANS_BUF_SIZE 100
#define TRANS_WDW_SIZE 3
#define TIMEOUT_TIME 20

/* Modular counting macros */
#define INC(i) MOD_INC(i, TRANS_BUF_SIZE)

/* Class static variables */
static char* dataMessage;
static char* msgEnd;
static uint8_t sequenceNumber;

static WindowBuffer_ts wBuffer;

static TransportMessage_ts messages[MAX_NODES][MAX_MSG_ID];
static int output;

/**************************** FUNCTION PROTOTYPES *****************************/
static void messageClear(TransportMessage_ts*);
static void sequenceNumberToTwoByteAscii(uint8_t []);
static void receiveDataMessage(TransportMessage_ts*);
static void receiveAckMessage(TransportMessage_ts*);

/*************************** FUNCTION DEFINITIONS *****************************/

/**
 * Converts sequenceNumber to a two digit base 10 number in ASCII and puts in d.
 */
static void
sequenceNumberToTwoByteAscii(uint8_t d[]) {
	d[0] = TO_ASCII(sequenceNumber / 10);
	d[1] = TO_ASCII(sequenceNumber % 10);
}

static uint8_t
sequenceNumToInt(uint8_t d[]) {
	return FROM_ASCII(d[0]) * 10 + FROM_ASCII(d[1]);
}

/**
 * Splits the remaining data message into transport layer messages and puts them
 * in the window buffer.
 */
static void
message_copy(void* dest, void* arg) {
	memcpy(dest, arg, sizeof(TransportMessage_ts));
}

static void
bufferMsg(void) {
	if(dataMessage == NULL)
		return;

	while(dataMessage < msgEnd) {
		TransportMessage_ts msg = {0};

		msg.byte[TRANS_MSG_TYPE] = 'd';
		msg.byte[TRANS_SRCID] = TO_ASCII(SRC_ID);
		msg.byte[TRANS_DSTID] = TO_ASCII(DST_ID);
		sequenceNumberToTwoByteAscii(msg.byte + TRANS_SEQNUM);
		INC(sequenceNumber);

		size_t bytesToCopy = MIN(TRANS_MAX_DATA, msgEnd - dataMessage);
		memcpy(msg.byte + TRANS_DATA, dataMessage, bytesToCopy);

		if(windowbuffer_addElement(&wBuffer, &msg, message_copy) == NULL) {
			printf("Could not copy message\n");
			return;
		}

		dataMessage += bytesToCopy;
	}
}

void
transport_printMessage(void* arg) {
	TransportMessage_ts* msg = (TransportMessage_ts*)arg;

	if(msg != NULL) {
		printf("MSG TYPE: %c SRC_ID: %c DST_ID: %c SEQ_NUM: %c%c\n",
			msg->byte[TRANS_MSG_TYPE], msg->byte[TRANS_SRCID],
			msg->byte[TRANS_DSTID], msg->byte[TRANS_SEQNUM],
			msg->byte[TRANS_SEQNUM + 1]
		);

		for(int i = TRANS_DATA; i < TRANS_MSG_SIZE && msg->byte[i]; i++)
			putchar(msg->byte[i]);
		putchar('\n');
	}
}

static void
messageClear(TransportMessage_ts* msg) {
	memset(msg->byte + TRANS_DATA, 0, TRANS_MAX_DATA);
}

bool
transport_init(char* msg) {
	char fileName[14] = {0};
	memcpy(fileName, "NodeXreceived", 14);
	fileName[4] = TO_ASCII(SRC_ID);

	if((output = open(fileName, O_CREAT | O_WRONLY, S_IRWXU)) < 0) {
		printf("Error opening %s\n", fileName);
		return false;
	}

	dataMessage = msg;
	msgEnd = dataMessage != NULL ? dataMessage + strlen(dataMessage) : NULL;

	if(windowbuffer_init(&wBuffer, TRANS_BUF_SIZE, sizeof(TransportMessage_ts),
	TRANS_WDW_SIZE, TIMEOUT_TIME) == NULL) {
		printf("Error allocating window buffer in transport layer\n");
		return false;
	}

	bufferMsg();
//	windowbuffer_print(&wBuffer, transport_printMessage);

	return true;
}

void
transport_sendString(void) {
	/* If the message was not completely buffered previously then continue */
	if(dataMessage != NULL && *dataMessage != '\0')
		bufferMsg();

	/* Array of indices of messages in the window buffer to send */
	uint8_t transmissionIdx[TRANS_WDW_SIZE] = {0};

	/* Fill the transmissionIdx array and get number of messages to send */
	uint8_t msgsToSend =
		windowbuffer_sendReadyElements(&wBuffer, transmissionIdx,
			TRANS_WDW_SIZE, now);

	/* Send messages to network layer if there are any */
	network_receiveFromTransport(wBuffer.buffer, transmissionIdx, msgsToSend);
}

uint8_t
transport_messageOffset(TransportMessage_ts* msg) {
	uint8_t offset = TRANS_DATA - 1;
	for(int i = TRANS_DATA; i < TRANS_MSG_SIZE; i++) {
		if(msg->byte[i] == 0)
			break;
		else
			offset++;
	}

	return TO_ASCII(offset);
}

void
transport_terminate(void) {
	windowbuffer_free(&wBuffer);
	close(output);
}

void
transport_receiveFromNetwork(NetworkPacket_ts* packet) {
	TransportMessage_ts msg = {0};

	memcpy(&msg, packet->byte + NW_DATA, sizeof(TransportMessage_ts));

	if(msg.byte[TRANS_MSG_TYPE] == 'a')
		receiveAckMessage(&msg);
	else
		receiveDataMessage(&msg);
}

void
receiveAckMessage(TransportMessage_ts* msg) {
	windowbuffer_acknowledge(&wBuffer,
		sequenceNumToInt(msg->byte + TRANS_SEQNUM));
}

void
receiveDataMessage(TransportMessage_ts* msg) {
	uint8_t src = FROM_ASCII(msg->byte[TRANS_SRCID]);
	uint8_t sqn = sequenceNumToInt(msg->byte + TRANS_SEQNUM);
	message_copy(&messages[src][sqn], msg);
	
/*	printf("NODE %d RECEIVED MESSAGE FROM %c\n",
		SRC_ID, msg->byte[TRANS_SRCID]);
	transport_printMessage(msg);
*/
	TransportMessage_ts ack = {0};
	ack.byte[TRANS_MSG_TYPE] = 'a';
	ack.byte[TRANS_SRCID] = TO_ASCII(SRC_ID);
	ack.byte[TRANS_DSTID] = msg->byte[TRANS_SRCID];
	ack.byte[TRANS_SEQNUM] = msg->byte[TRANS_SEQNUM];
	ack.byte[TRANS_SEQNUM + 1] = msg->byte[TRANS_SEQNUM + 1];

	network_addMessageToBuffer(&ack);
}

void
transport_writeMessages(void) {
	for(uint8_t i = 0; i < MAX_NODES; i++) {
		uint8_t preamble[17] = {0};
		memcpy(preamble, "From X received: ", 17);
		preamble[5] = TO_ASCII(i);

		uint16_t bufIdx = 0;
		uint8_t buffer[MAX_MSG_ID * TRANS_MAX_DATA] = {0};
		for(uint8_t j = 0; j < MAX_MSG_ID; j++) {
			/* If no more messages then stop */
			if(messages[i][j].byte[TRANS_MSG_TYPE]) {
				for(uint8_t k = TRANS_DATA; k < TRANS_MSG_SIZE &&
				messages[i][j].byte[k]; k++)
					buffer[bufIdx++] = messages[i][j].byte[k];
			}
		}

		if(bufIdx) {
			write(output, preamble, 17);
			buffer[bufIdx++] = '\n';
			write(output, buffer, bufIdx);
		}
	}
}

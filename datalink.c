/**
 * dll.c
 * Author: Joshua Newhouse 15 April 2018
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "node.h"
#include "circularbuffer.h"
#include "windowbuffer.h"
#include "datalink.h"
#include "network.h"
#include "transport.h"

/* Datalink Layer Constants and Macros */
#define DLL_BUF_SIZE 100
#define DLL_WDW_SIZE 20
#define DLL_TIMEOUT_TIME 5

/* Modular counting macros */
#define INC(i) MOD_INC(i, DLL_BUF_SIZE)

/* Frame bytes */
#define FRAME_BEGIN 'F'
#define FRAME_END 'E'
#define FRAME_ESCAPE 'X'

static char channelFileName[] = {
	'f', 'r', 'o', 'm', 'X', 't', 'o', 'X', '\0'
};
static int channel[MAX_NODES][2];
static WindowBuffer_ts wBuffer;
static CircularBuffer_ts recBuffer;
static uint8_t seqNum;

/************************ Local Function Declarations *************************/

static void sequenceNumberToTwoByteAscii(uint8_t []);
static uint8_t sequenceNumToInt(uint8_t []);
static void writeToChannels(void);
static void frame_writeToFile(DatalinkFrame_ts*, int);
static void receiveAckFrame(DatalinkFrame_ts*);
static void receiveDataFrame(DatalinkFrame_ts*);
static void receiveDataFrame(DatalinkFrame_ts*);
static uint8_t frame_readDataFrame(uint8_t*, uint16_t, uint16_t);
static uint8_t frame_readAckFrame(uint8_t*, uint16_t, uint16_t);

/**************************** Function Definitions ****************************/

/**
 * Converts sequenceNumber to a two digit base 10 number in ASCII and puts in d.
 */
static void
sequenceNumberToTwoByteAscii(uint8_t d[]) {
	d[0] = TO_ASCII(seqNum / 10);
	d[1] = TO_ASCII(seqNum % 10);

	INC(seqNum);
}

static uint8_t
sequenceNumToInt(uint8_t d[]) {
	return FROM_ASCII(d[0]) * 10 + FROM_ASCII(d[1]);
}

bool datalink_init(void) {
	int fd;
	for(uint8_t i = 0; i < MAX_NODES; i++) {
		if(neighborNode[i]) {
			channelFileName[4] = TO_ASCII(SRC_ID);
			channelFileName[7] = TO_ASCII(i);

			fd = open(channelFileName, O_TRUNC | O_CREAT | O_WRONLY, S_IRWXU);
			if(fd < 0) {
				printf("Error opening %s\n", channelFileName);
				return false;
			}
			channel[i][OBND_CHANNEL] = fd;

			channelFileName[4] = TO_ASCII(i);
			channelFileName[7] = TO_ASCII(SRC_ID);

			fd = open(channelFileName, O_CREAT | O_RDONLY, S_IRWXU);
			if(fd < 0) {
				printf("Error opening %s\n", channelFileName);
				return false;
			}
			channel[i][IBND_CHANNEL] = fd;
		}
		else {
			channel[i][OBND_CHANNEL] = -1;
			channel[i][IBND_CHANNEL] = -1;
		}
	}

	windowbuffer_init(&wBuffer, DLL_BUF_SIZE, sizeof(DatalinkFrame_ts),
		DLL_WDW_SIZE, DLL_TIMEOUT_TIME);

	circularbuffer_init(&recBuffer, DLL_BUF_SIZE, sizeof(DatalinkFrame_ts));

	return true;
}

static void
frame_copy(void* dest, void* arg) {
	memcpy(dest, arg, sizeof(DatalinkFrame_ts));
}

static void
printFrame(void* arg) {
	DatalinkFrame_ts* f = (DatalinkFrame_ts*)arg;

	printf("FRAME:\nFRAMETYPE: %c%c%c%c CHANNEL: %c SEQ: %c%c\n",
		f->byte[DL_LBL], f->byte[DL_LBL + 1], f->byte[DL_LBL + 2],
		f->byte[DL_LBL + 3], f->byte[DL_CHL], f->byte[DL_SEQ],
		f->byte[DL_SEQ + 1]
	);

	network_printPacket((NetworkPacket_ts*)(f->byte + DL_DATA));
}

void
datalink_receiveFromNetwork(CircularBuffer_ts* packetBuffer) {
	NetworkPacket_ts* packet;

	while((packet = circularbuffer_getNext(packetBuffer)) != NULL) {
		DatalinkFrame_ts frame = {0};

		memcpy(frame.byte + DL_LBL, "data", 4);
		frame.byte[DL_CHL] = TO_ASCII(SRC_ID);
		sequenceNumberToTwoByteAscii(frame.byte + DL_SEQ);

		memcpy(frame.byte + DL_DATA, packet, sizeof(NetworkPacket_ts));		

		if(windowbuffer_addElement(&wBuffer, &frame, frame_copy) == NULL)
			printf("Could not add frame\n");
	}

//	printf("FRAMES RECEIVED FROM NETWORK:\n");
//	windowbuffer_print(&wBuffer, printFrame);

	writeToChannels();
}

static void
frame_writeAckToFile(DatalinkFrame_ts* frame, int fd) {
	uint8_t writeBuffer[1 + DL_HDR_SIZE + 1] = {FRAME_BEGIN};

	memcpy(writeBuffer + 1, frame->byte, DL_HDR_SIZE);
	writeBuffer[DL_DATA + 1] = FRAME_END;

	write(fd, writeBuffer, 1 + DL_HDR_SIZE + 1);
}

static void
frame_writeDataToFile(DatalinkFrame_ts* frame, int fd) {
	uint8_t writeBuffer[1 + DL_FRM_SIZE + TRANS_MAX_DATA + 1] = {FRAME_BEGIN};
	memcpy(writeBuffer + 1, frame->byte, DL_FRM_SIZE - TRANS_MAX_DATA);

	int i = DL_DATA + NW_DATA + TRANS_DATA;
	int j = 1 + DL_FRM_SIZE - TRANS_MAX_DATA;
	int jLimit = 1 + DL_FRM_SIZE + TRANS_MAX_DATA;

	while(i < DL_FRM_SIZE && j < jLimit && frame->byte[i] != 0) {
		uint8_t temp = frame->byte[i];
		switch(temp) {
		case FRAME_ESCAPE: case FRAME_BEGIN: case FRAME_END:
			writeBuffer[j++] = FRAME_ESCAPE;
		default:
			writeBuffer[j++] = frame->byte[i++];
		}
	}

	writeBuffer[j] = FRAME_END;

	write(fd, writeBuffer, j + 1);	
}

static void
writeToChannels(void) {
	/* Array of indices of messages in the window buffer to send */
	uint8_t frameIndices[DLL_WDW_SIZE] = {0};

	/* Fill the transmissionIdx array and get number of messages to send */
	uint8_t framesToSend =
		windowbuffer_sendReadyElements(&wBuffer, frameIndices, DLL_WDW_SIZE,
			now);

	DatalinkFrame_ts* frame = (DatalinkFrame_ts*)wBuffer.buffer;
	for(uint8_t i = 0; i < MAX_NODES; i++) {
		int fd = channel[i][OBND_CHANNEL];
		if(fd != -1) {
			for(uint8_t j = 0; j < framesToSend; j++) {
				frame_writeDataToFile(&frame[frameIndices[j]], fd);
			}
		}
	}
}

void datalink_terminate(void) {
	for(uint8_t i = 0; i < MAX_NODES; i++) {
		if(channel[i][OBND_CHANNEL] != -1)
			close(channel[i][OBND_CHANNEL]);
		if(channel[i][IBND_CHANNEL] != -1)
			close(channel[i][IBND_CHANNEL]);
	}

	windowbuffer_free(&wBuffer);
	circularbuffer_free(&recBuffer);
}

#define READ_BUF_SIZE 1024

void datalink_receiveFromChannel(void) {
	uint8_t readBuffer[READ_BUF_SIZE] = {0};

	for(int n = 0; n < MAX_NODES; n++) {
		int fd = channel[n][IBND_CHANNEL];
		if(fd != -1) {
			ssize_t bytesRead;
			while((bytesRead = read(fd, readBuffer, READ_BUF_SIZE)) > 0) {
				uint16_t i = 0;
				while(i < bytesRead - 1 && readBuffer[i++] == FRAME_BEGIN) {
					if(readBuffer[i] == 'a')
						i = frame_readAckFrame(readBuffer, READ_BUF_SIZE, i);
					else
						i = frame_readDataFrame(readBuffer, READ_BUF_SIZE, i);
				}
			}
		}
	}

	network_receiveFromDatalink(&recBuffer);
}

static uint8_t
frame_readAckFrame(uint8_t* buf, uint16_t n, uint16_t i) {
//	printf("RECEIVED ACK FRAME: %d\n", SRC_ID);

	DatalinkFrame_ts frame = {0};
	memcpy(&frame, buf + i, DL_HDR_SIZE);

	windowbuffer_acknowledge(&wBuffer, sequenceNumToInt(frame.byte + DL_SEQ));

//	printFrame(&frame);

	return i + DL_HDR_SIZE + 1;
}

static uint8_t
frame_readDataFrame(uint8_t* buf, uint16_t n, uint16_t i) {
//	printf("RECEIVED DATA FRAME: %d\n", SRC_ID);

	DatalinkFrame_ts frame = {0};

	uint8_t j = DL_HDR_SIZE + NW_HDR_SIZE + TRANS_HDR_SIZE;
	memcpy(frame.byte, buf + i, j);

	i += j;
	while(i < n && buf[i] != FRAME_END && j < DL_FRM_SIZE) {
		uint8_t temp = buf[i];
		switch(temp) {
		case FRAME_ESCAPE:
			frame.byte[j++] = buf[++i];
			i++;
			break;
		default:
			frame.byte[j++] = buf[i++];
		}
	}

//	printFrame(&frame);

	receiveDataFrame(&frame);

	return i + 1;
}

static void
receiveDataFrame(DatalinkFrame_ts* frame) {
	circularbuffer_add(&recBuffer, frame, frame_copy);

	DatalinkFrame_ts ack = {0};
	memcpy(ack.byte + DL_LBL, "ack ", 4);
	ack.byte[DL_CHL] = TO_ASCII(SRC_ID);
	ack.byte[DL_SEQ] = frame->byte[DL_SEQ];
	ack.byte[DL_SEQ + 1] = frame->byte[DL_SEQ + 1];

	uint8_t dest = FROM_ASCII(frame->byte[DL_CHL]);
	frame_writeAckToFile(&ack, channel[dest][OBND_CHANNEL]);
}

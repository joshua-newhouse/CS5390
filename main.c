#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "node.h"
#include "datalink.h"
#include "network.h"
#include "transport.h"

/* State variables for this node */
uint8_t SRC_ID;
uint8_t DST_ID;

uint8_t neighborNode[MAX_NODES];

uint16_t now;
static uint16_t runtime;

static char* message;

static void printNodeState(void) {
	printf("Node Addr:\t%d\n", SRC_ID);
	printf("Dest Addr:\t%d\n", DST_ID);
	printf("Run time:\t%d\n", runtime);
	printf("Message:\t%s\n", message == NULL ? "N/A" : message);

	printf("Neighbors:\t");
	int i;
	for(i = 0; i < MAX_NODES; i++)
		if(neighborNode[i])
			printf("%d ", i);

	printf("\n");
}

static void configure(int argc, char* argv[]) {
	/* Get address, runtime, and destination address from CL */
	SRC_ID = atoi(argv[1]);
	runtime = atoi(argv[2]);
	DST_ID = atoi(argv[3]);

	/* Determine if there is a message to send on CL and get it */
	int hasMessage = SRC_ID != DST_ID;
	message = hasMessage ? argv[4] : NULL;

	/* Get the neighboring nodes */
	int i = hasMessage ? 5 : 4;
	for(; i < argc; i++) {
		int temp = atoi(argv[i]);
		if(temp < MAX_NODES && temp >= 0)
			neighborNode[temp] = 1;
		else
			printf("Error: Invalid node address %d.\n", temp);
	}
}

int main(int argc, char* argv[]) {
	if(argc < 5) {
		printf("Error: Must have at least 5 arguments.  TERMINATING...\n");
		exit(EXIT_FAILURE);
	}

	/* Configure this node */
	configure(argc, argv);
//	printNodeState();

	/* Initilize Network Layers */
	if(!transport_init(message)) {
		printf("Transport init fail: %d\n", SRC_ID);
		exit(EXIT_FAILURE);
	}

	if(!network_init()) {
		printf("Network init fail: %d\n", SRC_ID);
		exit(EXIT_FAILURE);
	}

	if(!datalink_init()) {
		printf("Datalink init fail: %d\n", SRC_ID);
		exit(EXIT_FAILURE);
	}

	/* Run the node */
	for(; now < runtime; now++) {
		transport_sendString();
		datalink_receiveFromChannel();
		sleep(1);
	}

	transport_writeMessages();

	/* Terminate this node */
	transport_terminate();
	network_terminate();
	datalink_terminate();
	exit(EXIT_SUCCESS);
}

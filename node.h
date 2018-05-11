#ifndef NODE_H
#define NODE_H

#include <inttypes.h>

#define MAX_NODES 10

#define TO_ASCII(i) ((i) + '0')
#define FROM_ASCII(i) ((i) - '0')
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MOD_INC(i, mod) (i = (i + 1) % (mod))
#define MOD_PLUS1(i, mod) (((i) + 1) % (mod))

#define TRANS_MAX_DATA 5
#define TRANS_HDR_SIZE 5
#define TRANS_MSG_SIZE (TRANS_HDR_SIZE + TRANS_MAX_DATA)

#define MAX_MSG_ID 100
#define NW_HDR_SIZE 5
#define NW_PKT_SIZE (NW_HDR_SIZE + TRANS_MSG_SIZE)

#define DL_HDR_SIZE 7
#define DL_FRM_SIZE (DL_HDR_SIZE + NW_PKT_SIZE)

extern uint8_t SRC_ID;
extern uint8_t DST_ID;
extern uint8_t neighborNode[];
extern uint16_t now;

#endif

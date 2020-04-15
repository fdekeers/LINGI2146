/**
 * Defines data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/cc2420/cc2420.h"
#include "sys/etimer.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"

#include "hashmap.h"


///////////////////
///  CONSTANTS  ///
///////////////////

// Constants for offset (in bytes) in packets
#define OFFSET_TYPE  0
#define OFFSET_TIME  1
#define OFFSET_ADDR1 2
#define OFFSET_ADDR2 3

// Constants for runicast sending functions
#define SENT       1
#define NOT_SENT  -1
#define NO_PARENT -2

// Threshold to change parent (in dB)
#define RSS_THRESHOLD 3

// Maximum number of retransmissions for reliable unicast transport
#define MAX_RETRANSMISSIONS 4

// Values for the different types of RPL control messages
const uint8_t DIS;
const uint8_t DIO;
const uint8_t DAO;



////////////////////
///  DATA TYPES  ///
////////////////////

// Represents the attributes of a mote
typedef struct mote {
	linkaddr_t* addr;
	uint8_t in_dodag;
	uint8_t rank;
	struct mote* parent;
	signed char parent_rss;
	hashmap_map* routing_table;
} mote_t;



///////////////////
///  FUNCTIONS  ///
///////////////////

long current_time();

/**
 * Initializes the attributes of a mote.
 */
void init_mote(mote_t *mote);

/**
 * Initializes the attributes of a root mote.
 */
void init_root(mote_t *mote);

/**
 * Initializes the parent of a mote.
 */
void init_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss);

/**
 * Adds a child to this mote
 */
void add_child(mote_t *mote, const linkaddr_t *child_addr, uint8_t child_rank);

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn);

/**
 * Broadcasts a DIO message, containing the rank of the node.
 */
void send_DIO(struct broadcast_conn *conn, mote_t *mote);

/**
 * Selects the parent, if it has a better rss.
 */
uint8_t choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss);

/**
 * Sends a DAO message to the parent of this node.
 */
void send_DAO(struct runicast_conn *conn, mote_t *mote);

/**
 * Forwards the DAO message, with source address child_addr, to the parent of this node.
 */
void forward_DAO(struct runicast_conn *conn, mote_t *mote, linkaddr_t child_addr);

/**
 * Called when a DAO packet is received.
 */
void receive_DAO(struct runicast_conn *conn);

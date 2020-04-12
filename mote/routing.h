/**
 * Defines data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/cc2420/cc2420.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"



//////////////////
//  DATA TYPES  //
//////////////////

// Values for the different types of RPL control messages
const uint8_t DIS;
const uint8_t DIO;
const uint8_t DAO;

typedef struct mote {
	linkaddr_t* addr;
	uint8_t in_dodag;
	uint8_t rank;
	struct mote* parent;
	linkaddr_t* child_addr;
} mote_t;



/////////////////
//  FUNCTIONS  //
/////////////////

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
void init_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank);

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
 * Selects the parent
 */
void choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss);

/**
 * Sends a DAO message to the parent of this node.
 */
void send_DAO(struct runicast_conn *conn, mote_t *mote);

/**
 * Called when a DAO packet is received.
 */
void receive_DAO(struct runicast_conn *conn);

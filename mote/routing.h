/**
 * Defines data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"

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

typedef struct parent_mote {
	linkaddr_t* addr;
	uint8_t rank;
	signed char rss;
} parent_t;


/////////////////
//  FUNCTIONS  //
/////////////////

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn);

/**
 * Broadcasts a DIO message, containing the rank of the node.
 */
void send_DIO(struct broadcast_conn *conn, uint8_t rank);

/**
 * Called when a DIS packet is received.
 */
void receive_DIS(uint8_t in_dodag, struct broadcast_conn *conn, uint8_t rank);

/**
 * Called when a DIO packet is received.
 */
void receive_DIO(parent_t* parent, uint8_t rank_recv, signed char rss);

/**
 * Called when a DAO packet is received.
 */
void receive_DAO();
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


/////////////////
//  FUNCTIONS  //
/////////////////

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn);

/**
 * Called when a DIS packet is received.
 */
void receive_DIS();

/**
 * Called when a DIO packet is received.
 */
void receive_DIO();

/**
 * Called when a DAO packet is received.
 */
void receive_DAO();
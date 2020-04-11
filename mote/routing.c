/**
 * Data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "routing.h"

//////////////////
//  DATA TYPES  //
//////////////////

// Values for the different types of RPL control messages
const uint8_t DIS = 0;
const uint8_t DIO = 1;
const uint8_t DAO = 2;


/////////////////
//  FUNCTIONS  //
/////////////////

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn) {

	size_t size = sizeof(uint8_t);

	uint8_t* data = malloc(size);
	*data = DIS;

	packetbuf_copyfrom((void*) data, size);
	broadcast_send(conn);
	printf("DIS packet broadcasted.\n");

}

/**
 * Called when a DIS packet is received.
 */
void receive_DIS() {
	printf("DIS packet received.\n");
}

/**
 * Called when a DIO packet is received.
 */
void receive_DIO() {
	printf("DIO packet received.\n");
}

/**
 * Called when a DAO packet is received.
 */
void receive_DAO() {
	printf("DAO packet received.\n");
}
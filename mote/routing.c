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

	uint8_t* data = (uint8_t*) malloc(size);
	*data = DIS;

	packetbuf_copyfrom((void*) data, size);
	broadcast_send(conn);
	printf("DIS packet broadcasted.\n");

}

/**
 * Broadcasts a DIO message, containing the rank of the node.
 */
void send_DIO(struct broadcast_conn *conn, uint8_t rank) {

	size_t size = sizeof(uint8_t);

	uint8_t* data = (uint8_t*) malloc(size);
	*data = DIO;
	*(data+1) = rank;

	packetbuf_copyfrom((void*) data, size*2);
	broadcast_send(conn);
	printf("DIO packet broadcasted, rank = %d\n", rank);

}

/**
 * Called when a DIS packet is received.
 */
void receive_DIS(uint8_t in_dodag, struct broadcast_conn *conn, uint8_t rank) {
	printf("DIS packet received.\n");

	// If the node is already in a DODAG, send DIO packet
	if (in_dodag) {
		send_DIO(conn, rank);
	}

}

/**
 * Called when a DIO packet is received.
 */
void receive_DIO(parent_t* parent, uint8_t rank_recv, signed char rss) {
	printf("DIO packet received.\n");

	
}

/**
 * Called when a DAO packet is received.
 */
void receive_DAO() {
	printf("DAO packet received.\n");
}
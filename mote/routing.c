/**
 * Data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "routing.h"


///////////////////
///  CONSTANTS  ///
///////////////////

// Values for the different types of RPL control messages
const uint8_t DIS = 0;
const uint8_t DIO = 1;
const uint8_t DAO = 2;

// Size of control messages
const size_t DIS_size = sizeof(DIS_message_t);
const size_t DIO_size = sizeof(DIO_message_t);
const size_t DAO_size = sizeof(DAO_message_t);



///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Initializes the attributes of a mote.
 */
void init_mote(mote_t *mote) {

	// Set the Rime address
	linkaddr_copy(&(mote->addr), &linkaddr_node_addr);

	// Initialize routing table
	mote->routing_table = hashmap_new();

	mote->in_dodag = 0;
	mote->rank = 0;

}

/**
 * Initializes the attributes of a root mote.
 */
void init_root(mote_t *mote) {

	// Set the Rime address
	linkaddr_copy(&(mote->addr), &linkaddr_node_addr);

	// Initialize routing table
	mote->routing_table = hashmap_new();

	mote->in_dodag = 1;
	mote->rank = 0;
}

/**
 * Initializes the parent of a mote.
 */
void init_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss) {

	// Set the Rime address
	mote->parent = (mote_t*) malloc(sizeof(mote_t));
	linkaddr_copy(&(mote->parent->addr), parent_addr);

	mote->parent->in_dodag = 1;
	mote->parent->rank = parent_rank;
	mote->parent_rss = rss;

	// Update the attributes of the mote
	mote->in_dodag = 1;
	mote->rank = parent_rank + 1;

}

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn) {

	DIS_message_t *message = (DIS_message_t*) malloc(DIS_size);
	message->type = DIS;

	packetbuf_copyfrom((void*) message, DIS_size);
	free(message);
	broadcast_send(conn);
	printf("DIS packet broadcasted.\n");

}

/**
 * Broadcasts a DIO message, containing the rank of the node.
 */
void send_DIO(struct broadcast_conn *conn, mote_t *mote) {

	uint8_t rank = mote->rank;

	DIO_message_t *message = (DIO_message_t*) malloc(DIO_size);
	message->type = DIO;
	message->rank = rank;

	packetbuf_copyfrom((void*) message, DIO_size);
	free(message);
	broadcast_send(conn);
	printf("DIO packet broadcasted, rank = %d\n", rank);

}

/**
 * Sends a DAO message to the parent of this node.
 */
void send_DAO(struct runicast_conn *conn, mote_t *mote) {

	if (mote->parent == NULL) {
		printf("Root mote. DAO not forwarded.\n");
	} else {

		DAO_message_t *message = (DAO_message_t*) malloc(DAO_size);
		message->type = DAO;
		message->src_addr = mote->addr;

		packetbuf_copyfrom((void*) message, DAO_size);
		free(message);

		runicast_send(conn, &(mote->parent->addr), MAX_RETRANSMISSIONS);
		printf("DAO packet sent to parent at addr %d.%d\n",
			mote->parent->addr.u8[0], mote->parent->addr.u8[1]);

	}

}

/**
 * Forwards a DAO message, with source address child_addr, to the parent of this node.
 */
void forward_DAO(struct runicast_conn *conn, mote_t *mote, linkaddr_t child_addr) {

	if (mote->parent == NULL) {
		printf("Root mote. DAO not forwarded.\n");
	} else {

		DAO_message_t *message = (DAO_message_t*) malloc(DAO_size);
		message->type = DAO;
		message->src_addr = child_addr;

		packetbuf_copyfrom((void*) message, DAO_size);
		free(message);
		
		runicast_send(conn, &(mote->parent->addr), MAX_RETRANSMISSIONS);
		printf("DAO packet forwarded to parent at addr %d.%d\n",
			mote->parent->addr.u8[0], mote->parent->addr.u8[1]);

	}

}

/**
 * Selects the parent. Returns 1 if the parent has changed.
 */
uint8_t choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss) {
	if (!mote->in_dodag || (rss > mote->parent_rss + RSS_THRESHOLD && mote->rank >= parent_rank)) {
		init_parent(mote, parent_addr, parent_rank, rss);
		printf("Parent set : Addr = %d.%d; Rank = %d\n",
			parent_addr->u8[0], parent_addr->u8[1], parent_rank);
		return 1;
	}
	else {
		printf("Already has a better parent !\n");
		return 0;
	}
}

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



///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Initializes the attributes of a mote.
 */
void init_mote(mote_t *mote) {

	// Set the Rime address
	mote->addr = (linkaddr_t*) malloc(sizeof(uint8_t)*LINKADDR_SIZE);
	linkaddr_copy(mote->addr, &linkaddr_node_addr);

	mote->in_dodag = 0;
	mote->rank = 0;

}

/**
 * Initializes the attributes of a root mote.
 */
void init_root(mote_t *mote) {

	// Set the Rime address
	mote->addr = (linkaddr_t*) malloc(sizeof(uint8_t)*LINKADDR_SIZE);
	linkaddr_copy(mote->addr, &linkaddr_node_addr);

	mote->in_dodag = 1;
	mote->rank = 0;
}

/**
 * Initializes the parent of a mote.
 */
void init_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss) {

	// Set the Rime address
	mote->parent = (mote_t*) malloc(sizeof(mote_t));
	mote->parent->addr = (linkaddr_t*) malloc(sizeof(uint8_t)*LINKADDR_SIZE);
	linkaddr_copy(mote->parent->addr, parent_addr);

	mote->parent->in_dodag = 1;
	mote->parent->rank = parent_rank;
	mote->parent_rss = rss;

	// Update the attributes of the mote
	mote->in_dodag = 1;
	mote->rank = parent_rank + 1;

}

/**
 * Adds a child to this mote
 */
void add_child(mote_t *mote, const linkaddr_t *child_addr, uint8_t child_rank) {

}

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
void send_DIO(struct broadcast_conn *conn, mote_t *mote) {

	size_t size = sizeof(uint8_t);
	uint8_t rank = mote->rank;

	uint8_t* data = (uint8_t*) malloc(size);
	*data = DIO;
	*(data+1) = rank;

	packetbuf_copyfrom((void*) data, size*2);
	broadcast_send(conn);
	printf("DIO packet broadcasted, rank = %d\n", rank);

}

/**
 * Sends a DAO message to the parent of this node.
 */
void send_DAO(struct runicast_conn *conn, mote_t *mote) {

	if (mote->parent == NULL) {
		printf("This mote is a root.\n");
	} else {
		size_t size = sizeof(uint8_t);
		uint8_t* data = (uint8_t*) malloc(size);
		*data = DAO;
		packetbuf_copyfrom(data, size);
		runicast_send(conn, mote->parent->addr, 4);
		printf("DAO packet send to parent at addr %d.%d\n",
			mote->parent->addr->u8[0], mote->parent->addr->u8[1]);
	}

}

/**
 * Selects the parent. Returns 1 if the parent has changed.
 */
uint8_t choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss) {
	if (!mote->in_dodag || rss > mote->parent_rss + RSS_THRESHOLD) {
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

/**
 * Called when a DAO packet is received.
 */
void receive_DAO(struct runicast_conn *conn) {
	printf("DAO packet received.\n");
}

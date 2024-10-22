/**
 * Data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "routing.h"


///////////////////
///  CONSTANTS  ///
///////////////////

// Values for the different types of RPL control messages
const uint8_t DIS = 2;
const uint8_t DIO = 3;
const uint8_t DAO = 4;
const uint8_t DATA = 0;
const uint8_t OPEN = 1;

const uint8_t UP = 0;
const uint8_t DOWN = 1;

// Size of control messages
const size_t DIS_size = sizeof(DIS_message_t);
const size_t DIO_size = sizeof(DIO_message_t);
const size_t DAO_size = sizeof(DAO_message_t);
const size_t DATA_size = sizeof(DATA_message_t);
const size_t OPEN_size = sizeof(OPEN_message_t);



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

	if (!mote->routing_table) {
		printf("init_mote() of mote with address %u.%u : could not allocate enough memory\n", (mote->addr).u8[0], (mote->addr).u8[1]);
		exit(-1);
	}

	mote->in_dodag = 0;
	mote->rank = INFINITE_RANK;

}

/**
 * Initializes the attributes of a root mote.
 */
void init_root(mote_t *mote) {

	// Set the Rime address
	linkaddr_copy(&(mote->addr), &linkaddr_node_addr);

	// Initialize routing table
	mote->routing_table = hashmap_new();

	if (!mote->routing_table) {
		printf("init_root() of mote with address %u.%u : could not allocate enough memory\n", (mote->addr).u8[0], (mote->addr).u8[1]);
		exit(-1);
	}

	mote->in_dodag = 1;
	mote->rank = 0;
}

/**
 * Initializes the parent of a mote.
 */
void init_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss) {

	// Set the Rime address
	mote->parent = (parent_t*) malloc(sizeof(parent_t));
	linkaddr_copy(&(mote->parent->addr), parent_addr);

	// Set the attributes of the parent
	mote->parent->rank = parent_rank;
	mote->parent->rss = rss;

	// Update the attributes of the mote
	mote->in_dodag = 1;
	mote->rank = parent_rank + 1;

}

/**
 * Updates the attributes of the parent of a mote.
 * Returns 1 if the rank of the parent has changed, 0 if it hasn't changed.
 */
uint8_t update_parent(mote_t *mote, uint8_t parent_rank, signed char rss) {
	mote->parent->rss = rss;
	if (parent_rank != mote->parent->rank) {
		mote->parent->rank = parent_rank;
		mote->rank = parent_rank + 1;
		return 1;
	} else {
		return 0;
	}
}

/**
 * Changes the parent of a mote
 */
void change_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss) {

	// Set the Rime address
	linkaddr_copy(&(mote->parent->addr), parent_addr);

	// Set the attributes of the parent
	mote->parent->rank = parent_rank;
	mote->parent->rss = rss;

	// Update the rank of the mote
	mote->rank = parent_rank + 1;

}

/**
 * Detaches a mote from the DODAG.
 * Deletes the parent, and sets in_dodag and rank to 0.
 * Restart its routing table.
 */
void detach(mote_t *mote) {
	if (mote->in_dodag) { // No need to detach the mote if it isn't already in the DODAG
		free(mote->parent);
		hashmap_free(mote->routing_table);
		mote->in_dodag = 0;
		mote->rank = INFINITE_RANK;
		mote->routing_table = hashmap_new();
	}
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

}

/**
 * Sends a DAO message to the parent of this node.
 */
void send_DAO(struct runicast_conn *conn, mote_t *mote) {

	DAO_message_t *message = (DAO_message_t*) malloc(DAO_size);
	message->type = DAO;
	message->src_addr = mote->addr;

	packetbuf_copyfrom((void*) message, DAO_size);
	free(message);

	runicast_send(conn, &(mote->parent->addr), MAX_RETRANSMISSIONS);

}

/**
 * Forwards a DAO message, to the parent of this node.
 */
void forward_DAO(struct runicast_conn *conn, DAO_message_t *message, mote_t *mote) {
	packetbuf_copyfrom((void*) message, DAO_size);
	runicast_send(conn, &(mote->parent->addr), MAX_RETRANSMISSIONS);
}

/**
 * Returns 1 if the potential parent is better than the current parent, 0 otherwise.
 * A parent is better than another if it has a lower rank, or if it has the same rank
 * and a better signal strength (RSS), with a small threshold to avoid changing all the time
 * in an unstable network.
 */
uint8_t is_better_parent(mote_t *mote, uint8_t parent_rank, signed char rss) {
	uint8_t lower_rank = parent_rank < mote->parent->rank;
	uint8_t same_rank = parent_rank == mote->parent->rank;
	uint8_t better_rss = rss > mote->parent->rss + RSS_THRESHOLD;
	return lower_rank || (same_rank && better_rss);
}

/**
 * Selects the parent. Returns a code depending on if the parent has changed or not.
 */
uint8_t choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss) {
	if (!mote->in_dodag) {
		// Mote not in DODAG yet, initialize parent
		init_parent(mote, parent_addr, parent_rank, rss);
		return PARENT_NEW;
	} else if (is_better_parent(mote, parent_rank, rss)) {
		// Better parent found, change parent
		change_parent(mote, parent_addr, parent_rank, rss);
		return PARENT_CHANGED;
	} else {
		// Already has a better parent
		return PARENT_NOT_CHANGED;
	}
}

/**
 * Sends a DATA message, containing a random value, to the parent of the mote.
 */
void send_DATA(struct runicast_conn *conn, mote_t *mote) {

	DATA_message_t *message = (DATA_message_t*) malloc(DATA_size);
	message->type = DATA;
	message->src_addr = mote->addr;
	message->data = (uint16_t) (random_rand() % 501); // US A.Q.I. goes from 0 to 500

	packetbuf_copyfrom((void*) message, DATA_size);
	free(message);

	runicast_send(conn, &(mote->parent->addr), MAX_RETRANSMISSIONS);
}

/**
 * Forwards a DATA message to the parent of the mote.
 */
void forward_DATA(struct runicast_conn *conn, DATA_message_t *message, mote_t *mote) {
	packetbuf_copyfrom((void*) message, DATA_size);
	runicast_send(conn, &(mote->parent->addr), MAX_RETRANSMISSIONS);
}

/**
 * Sends an OPEN message to the sensor mote with address dst_addr, by sending it
 * to the next-hop address in the routing table.
 */
void send_OPEN(struct runicast_conn *conn, linkaddr_t dst_addr, mote_t *mote) {
	// Address of the next-hop mote towards destination
	linkaddr_t next_hop;
	if (hashmap_get(mote->routing_table, dst_addr, &next_hop) == MAP_OK) {
		// Node is correctly in the routing table
		OPEN_message_t* message = (OPEN_message_t*) malloc(OPEN_size);
		message->type = OPEN;
		message->dst_addr = dst_addr;
		packetbuf_copyfrom((void*) message, OPEN_size);
		runicast_send(conn, &next_hop, MAX_RETRANSMISSIONS);
	} else {
		// Destination mote wasn't present in routing table
		printf("Mote not in routing table.\n");
	}
}

/**
 * Forwards an OPEN message to the next hop mote on the path to the destination.
 */
void forward_OPEN(struct runicast_conn *conn, OPEN_message_t *message, mote_t *mote) {
	// Address of the next-hop mote towards destination
	linkaddr_t next_hop;
	if (hashmap_get(mote->routing_table, message->dst_addr, &next_hop) == MAP_OK) {
		// Forward to next_hop
		packetbuf_copyfrom((void*) message, OPEN_size);
		runicast_send(conn, &next_hop, MAX_RETRANSMISSIONS);
	} else {
		printf("Error in forwarding OPEN message.\n");
	}
}

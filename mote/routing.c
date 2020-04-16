/**
 * Data and functions useful for the routing protocol of the motes, based on RPL.
 */

#include "routing.h"


///////////////////
///  CONSTANTS  ///
///////////////////

// Values for the different types of RPL control messages
const uint8_t DIS = 1;
const uint8_t DIO = 2;
const uint8_t DAO = 3;
const uint8_t DLT = 4;

// Size of control messages
const size_t DIS_size = sizeof(DIS_message_t);
const size_t DIO_size = sizeof(DIO_message_t);
const size_t DAO_size = sizeof(DAO_message_t);
const size_t DLT_size = sizeof(DLT_message_t);



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

	// Initialize buffer of children
	mote->children = malloc(MAX_NB_CHILDREN*sizeof(child_mote_t));

	if (!mote->routing_table || !mote->children) {
		printf("init_mote() of mote with address %u.%u : could not allocate enough memory\n", (mote->addr).u8[0], (mote->addr).u8[1]);
		exit(-1);
	}

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

	// Initialize buffer of children
	mote->children = malloc(MAX_NB_CHILDREN*sizeof(child_mote_t));

	if (!mote->routing_table || !mote->children) {
		printf("init_mote() of mote with address %u.%u : could not allocate enough memory\n", (mote->addr).u8[0], (mote->addr).u8[1]);
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

	mote->parent->rank = parent_rank;
	mote->parent->rss = rss;

	// Update the attributes of the mote
	mote->in_dodag = 1;
	mote->rank = parent_rank + 1;

}

/**
 * Changes the parent of a mote
 */
void change_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss) {

	// Set the Rime address
	linkaddr_copy(&(mote->parent->addr), parent_addr);

	mote->parent->rank = parent_rank;
	mote->parent->rss = rss;

	// Update the rank of the mote
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
 * Selects the parent. Returns a code depending on if the parent has changed or not.
 */
uint8_t choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss) {
	if (!mote->in_dodag) {
		// Mote not in DODAG yet, initialize parent
		init_parent(mote, parent_addr, parent_rank, rss);
		printf("Parent set : Addr = %d.%d; Rank = %d\n",
			mote->parent->addr.u8[0], mote->parent->addr.u8[1], mote->parent->rank);
		return PARENT_INIT;
	} else if (rss > mote->parent->rss + RSS_THRESHOLD && mote->rank > parent_rank) {
		// Better parent found, change parent
		change_parent(mote, parent_addr, parent_rank, rss);
		printf("Parent changed to : Addr = %d.%d; Rank = %d\n",
			mote->parent->addr.u8[0], mote->parent->addr.u8[1], mote->parent->rank);
		return PARENT_CHANGED;
	} else {
		// Already has a better parent
		printf("Already has a better parent !\n");
		return PARENT_NOT_CHANGED;
	}
}

/**
 * Updates the timestamp of node having addr child_addr to the given time or adds the node if it didn't exist
 * Prints an error if a node should be added but no more space is available
 */
void update_timestamp(mote_t *mote, unsigned long time, linkaddr_t child_addr) {
	child_mote_t *runner = mote->children;
	int first_free_ind = -1;
	int i;
	for (i = 0; i < MAX_NB_CHILDREN; i++) {
		if (!runner->in_use) {
			if (first_free_ind == -1) {
				first_free_ind = i;
			}
		} else if (linkaddr_cmp(&(runner->addr), &(runner->addr))) {
			// linkaddr_cmp returns non-zero if equal
			runner->timestamp = time;
			return;
		}
	}
	if (first_free_ind == -1) {
		printf("ERROR update_timestamp for node %u.%u : Too many child nodes, could not add/update timestamp of node %u.%u\n", mote->addr.u8[0], mote->addr.u8[1], child_addr.u8[0], child_addr.u8[1]);
	} else {
		child_mote_t* new_child = (mote->children) + first_free_ind;
		new_child->in_use = 1;
		new_child->addr = child_addr;
		new_child->timestamp = time;
	}
}

/**
 * Removes children that did not send a message since a long time
 */
void remove_unresponding_children(mote_t *mote, unsigned long current_time) {
	child_mote_t *runner = mote->children;
	int i;
	for (i = 0; i < MAX_NB_CHILDREN; i++) {
		if (runner->in_use && current_time > runner->timestamp+TIMEOUT_CHILD) {
			// time shouldn't wrap. 2^32 ~= 50 000 days ~= 136 years
			// removing child data
			runner->in_use = 0;
			// TEMPORAIRE. TODO : on devrait mettre un compteur et chaque 5 min, on check le compteur
			// si compteur > 0 on traverse la hashmap pour remove l'entrée de l'enfant mort + les entrées dépendantes de lui
			// Problème ENCORE : si enfant 2e degré meurt et 3e degré aussi, comment peut-on delete
			// l'enfant du 3e degré... car next-hop = 1e degré seulement...
			// ou alors 1er degré remontera l'info !! bonne sol
			hashmap_remove(mote->routing_table, runner->addr);
		}
	}
}

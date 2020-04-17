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

// Infinite rank constant
#define INFINITE_RANK 255

// Constant for periodicity of messages [sec]
#define SEND_PERIOD    3
#define DELETE_PERIOD  10

// Constants for runicast sending functions
#define SENT       1
#define NOT_SENT  -1
#define NO_PARENT -2

// Return values for choose_parent function
#define PARENT_NOT_CHANGED  0
#define PARENT_INIT         1
#define PARENT_CHANGED      2

// Threshold to change parent (in dB)
#define RSS_THRESHOLD 3

// Maximum number of retransmissions for reliable unicast transport
#define MAX_RETRANSMISSIONS 4

// Maximum number of children for each mote
#define MAX_NB_CHILDREN 10

// Timeout [sec] to know when to forget a child
#define TIMEOUT_CHILD 10

// Values for the different types of RPL control messages
const uint8_t DIS;
const uint8_t DIO;
const uint8_t DAO;

// Size of control messages
const size_t DIS_size;
const size_t DIO_size;
const size_t DAO_size;



////////////////////
///  DATA TYPES  ///
////////////////////

// Represents the parent of a certain mote
// We use another struct since we don't need all the information of the mote struct
typedef struct parent_mote {
	linkaddr_t addr;
	uint8_t rank;
	signed char rss;
} parent_t;

// Represents the attributes of a mote child
typedef struct child_mote {
	linkaddr_t addr;
	uint8_t in_use;
	uint16_t timestamp;
} child_mote_t;

// Represents the attributes of a mote
typedef struct mote {
	linkaddr_t addr;
	uint8_t in_dodag;
	uint8_t rank;
	parent_t* parent;
	child_mote_t* children;
	hashmap_map* routing_table;
} mote_t;

// Represents a DIS control message
typedef struct DIS_message {
	uint8_t type;
} DIS_message_t;

// Represents a DIO control message
typedef struct DIO_message {
	uint8_t type;
	uint8_t rank;
} DIO_message_t;

// Represents a DAO control message
typedef struct DAO_message {
	uint8_t type;
	linkaddr_t src_addr;
} DAO_message_t;

// Represents a DLT control message, that is used to remove old children from the routing tables
typedef struct DLT_message {
	uint8_t type;
	linkaddr_t child_addr;
} DLT_message_t;



///////////////////
///  FUNCTIONS  ///
///////////////////

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
 * Changes the parent of a mote.
 */
void change_parent(mote_t *mote, const linkaddr_t *parent_addr, uint8_t parent_rank, signed char rss);

/**
 * Detaches a mote from the DODAG.
 * Deletes the parent, and sets in_dodag and rank to 0.
 */
void detach(mote_t *mote);

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn);

/**
 * Broadcasts a DIO message, containing the rank of the node.
 */
void send_DIO(struct broadcast_conn *conn, mote_t *mote);

/**
 * Sends a DAO message to the parent of this node.
 */
void send_DAO(struct runicast_conn *conn, mote_t *mote);

/**
 * Forwards the DAO message, with source address child_addr, to the parent of this node.
 */
void forward_DAO(struct runicast_conn *conn, mote_t *mote, linkaddr_t child_addr);

/**
 * Selects the parent, if it has a lower rank and a better rss
 */
uint8_t choose_parent(mote_t *mote, const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss);

/**
 * Updates the timestamp of node having addr child_addr to the given time or adds the node if it didn't exist
 * Prints an error if a node should be added but no more space is available
 */
void update_timestamp(mote_t *mote, unsigned long time, linkaddr_t child_addr);

/**
 * Sends a DLT message to the parent of this node, with child_addr as address of the child to remove
 * from the routing tables
 */
//void send_DLT(struct runicast_conn *conn, mote_t *mote, linkaddr_t child_addr);

/**
 * Removes children that did not send a message since a long time
 */
void remove_unresponding_children(mote_t *mote, unsigned long current_time);

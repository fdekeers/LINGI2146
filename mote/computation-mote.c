/**
 * Code for the sensor nodes
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"

#include "routing.h"
#include "trickle-timer.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"

///////////////////
///  CONSTANTS  ///
///////////////////

#define COMPUTED_BUFFER_FULL -1
#define OPEN_VALVE 1
#define CLOSE_VALVE 2
#define CANNOT_ADD_MOTE 3
#define MIN_NB_VALUES_COMPUTE 10 // minimum values needed to do the computation
#define MAX_NB_VALUES 30 // maximum number of values about the mote
#define MAX_NB_COMPUTED 5 // this node can compute the needed values for only this number of nodes
#define SLOPE_THRESHOLD -30 // definition of the threshold (in %) for which we should open valves to improve air quality

typedef struct computed_mote {
	linkaddr_t addr;
	unsigned long timestamp;
	uint8_t in_use;
	double values[MAX_NB_VALUES];
	uint8_t first_value_index;
	uint8_t first_free_value_index;
	//size is not needed if when created, we add an element directly
	// using this, if the 2 indexes are equal, it will mean that the buffer is full-
} computed_mote_t;

// Represents the attributes of this mote
mote_t mote;

// 1 if the mote has been created. Used to create the mote only once.
uint8_t created = 0;

// 1 if a message from the parent has been received in the timer interval.
uint8_t info_from_parent = 0;

// not so memory efficient but easy implementation. computation is done for the motes contained
computed_mote_t computed_motes[MAX_NB_COMPUTED]; 

// Trickle timer for the periodic messages
trickle_timer_t t_timer;

// Broadcast connection
static struct broadcast_conn broadcast;

// Reliable unicast (runicast) connection
static struct runicast_conn runicast;


//////////////////////////////
///  COMPUTATION OF MOTES  ///
//////////////////////////////

/**
 * Finds the index where the mote is located or could be added or COMPUTED_BUFFER_FULL if
 * the mote wasn't present and the buffer is full. It also deletes nodes that timed out
 */
int indexFind(linkaddr_t addr, unsigned long curr_time) {
	int index = COMPUTED_BUFFER_FULL;
	int i;
	for (i = 0; i < MAX_NB_COMPUTED; i++) {
		if (!computed_motes[i].in_use && index == COMPUTED_BUFFER_FULL) {
			index = i;
		}
		else if (linkaddr_cmp(&addr, &(computed_motes[i].addr))) {
			return i;
		}
		if (computed_motes[i].in_use && curr_time > computed_motes[i].timestamp+TIMEOUT) {
			// remove nodes that timed out
			computed_motes[i].in_use = 0;
			if (index == COMPUTED_BUFFER_FULL) {
				// this is the first unused node
				index = i;
			}
		}
	}
	return index;
}

/**
 * Returns the computed slope. If first_free_value_index and first_value_index are equal,
 * we consider that the buffer is of the maximum size. This function shouldn't be called
 * on an empty buffer
 */
int slope_value(int index_mote) {
	computed_mote_t *elem = &(computed_motes[index_mote]);
	int nb_values = ((elem->first_free_value_index+MAX_NB_VALUES) - elem->first_value_index) % MAX_NB_VALUES;
	if (nb_values == 0)
		nb_values = MAX_NB_VALUES; // happens when pointers are the same
	int x[nb_values];
	double *y = elem->values;
	int i;
	double sum_x = 0.0, sum_y = 0.0, sum_xx = 0.0, sum_xy = 0.0;
	for (i = 0; i < nb_values; i++) {
		x[i] = i;
		sum_x += i;
		float y_i = y[(elem->first_value_index+i)%MAX_NB_VALUES];
		sum_y += y_i;
		sum_xy = x[i]*y_i;
		sum_xx = x[i]*x[i];
	}
	float slope = (sum_x * sum_y - nb_values * sum_xy) / (sum_x * sum_x - nb_values * sum_xx);
	return ((int)(slope*100)) / 100;
}

/**
 * Adds the information received from the mote and returns whether the valve should be opened or not
 */
int add_and_check_valve(linkaddr_t addr, double quality_air_value) {
	unsigned long time = clock_seconds();
	int index_mote = indexFind(addr, time);
	if (index_mote == COMPUTED_BUFFER_FULL) {
		printf("Couldn't add mote %u.%u in the computation buffer\n", addr.u8[0], addr.u8[1]);
		return CANNOT_ADD_MOTE;
	}
	uint8_t enough_values = 0; // false
	computed_mote_t *elem = &(computed_motes[index_mote]);
	if (elem->in_use) {
		if (elem->first_value_index == elem->first_free_value_index) {
			// buffer is full
			enough_values = 1;
			elem->first_value_index = (elem->first_value_index + 1) % MAX_NB_VALUES; // increment so that this will be the next deleted value
		}
	} else {
		// adding a new node !
		elem->in_use = 1;
		elem->addr = addr;
		elem->first_value_index = 0;
		elem->first_free_value_index = 0;
		// we can add the value
	}
	elem->timestamp = time;
	(elem->values)[elem->first_free_value_index] = quality_air_value;
	elem->first_free_value_index = (elem->first_free_value_index + 1) % MAX_NB_VALUES;

	printf("Added mote %u.%u in the computation buffer\n", addr.u8[0], addr.u8[1]);

	if (((elem->first_free_value_index+MAX_NB_VALUES) - elem->first_value_index) % MAX_NB_VALUES > MIN_NB_VALUES_COMPUTE) {
		// enough values to compute whether we should open the valve
		enough_values = 1;
	}
	if (enough_values && slope_value(index_mote) <= SLOPE_THRESHOLD) {
		// TODO : > or < ? normally air quality should be high if quality is good so : slope < SLOPE_THRESHOLD with negative constant
		return OPEN_VALVE;
	}
	return CLOSE_VALVE;
}



/////////////////////////
///  CALLBACK TIMERS  ///
/////////////////////////

// Callback timer to send information messages
struct ctimer send_timer;

// Callback timer to send DAO messages to parent
struct ctimer DAO_timer;

// Callback timer to detach from lost parent
struct ctimer parent_timer;

// Callback timer to delete unresponsive children
struct ctimer children_timer;


/**
 * Callback function that will send the appropriate message when ctimer has expired.
 */
void send_callback(void *ptr) {

	// Send the appropriate message
	if (!mote.in_dodag) {
		send_DIS(&broadcast);
	} else {
		send_DIO(&broadcast, &mote);
		// Update the trickle timer
		trickle_update(&t_timer);
	}

	// Restart the timer with a new random value
	ctimer_set(&send_timer, trickle_random(&t_timer), send_callback, NULL);

}

/**
 * Callback function that will send a DAO message to the parent, if the mote is in the DODAG.
 */
void DAO_callback(void *ptr) {
	if (mote.in_dodag) {
		send_DAO(&runicast, &mote);
	}
	// Restart the timer with a new random value
	ctimer_set(&DAO_timer, trickle_random(&t_timer), DAO_callback, NULL);
}

/**
 * Resets the trickle timer and restarts the callback timers that use it.
 */
void reset_timers(trickle_timer_t *timer) {
	trickle_reset(&t_timer);
	ctimer_set(&send_timer, trickle_random(&t_timer),
		send_callback, NULL);
	ctimer_set(&DAO_timer, trickle_random(&t_timer),
		DAO_callback, NULL);
}

/**
 * Callback function that will detach from the DODAG if the parent is lost.
 */
void parent_callback(void *ptr) {

	// Detach from DODAG only if node was already in DODAG
	if (mote.in_dodag) {
		// Detach from DODAG
		detach(&mote);
		// Reset sending timers
		reset_timers(&t_timer);
	}

	// Restart the timer with a new random value
	ctimer_set(&parent_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
		parent_callback, NULL);
	
}

/**
 * Callback function that will delete unresponsive children from the routing table.
 */
void children_callback(void *ptr) {

	if (mote.in_dodag && hashmap_delete_timeout(mote.routing_table)) {
		// Children have been deleted, reset sending timers
		reset_timers(&t_timer);
	}

	// Restart the timer with a new random value
	ctimer_set(&children_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
			children_callback, NULL);

}



////////////////////////////
///  UNICAST CONNECTION  ///
////////////////////////////

/**
 * Callback function, called when an unicast packet is received
 */
void runicast_recv(struct runicast_conn *conn, const linkaddr_t *from, uint8_t seqno) {

	uint8_t* data = (uint8_t*) packetbuf_dataptr();
	uint8_t type = *data;

	if (type == DAO) {

		//printf("DAO message received from %u.%u\n", from->u8[0], from->u8[1]);

		DAO_message_t* message = (DAO_message_t*) packetbuf_dataptr();

		// Address of the mote that sent the DAO packet
		linkaddr_t child_addr = message->src_addr;

		int err = hashmap_put(mote.routing_table, child_addr, *from);
		if (err == MAP_NEW || err == MAP_UPDATE) {

			// Forward DAO message to parent
			forward_DAO(conn, &mote, child_addr);

			if (err == MAP_NEW) { // A new child was added to the routing table
				// Reset timers
				printf("New child added\n");
				reset_timers(&t_timer);

				/*if (linkaddr_cmp(&child_addr, from)) {
					// linkaddr_cmp returns non-zero if addresses are equal

					// update timestamp of the child now or add the new child
					unsigned long time = clock_seconds();
					update_timestamp(&mote, time, child_addr);
				}*/
			}
			
		} else {
			printf("Error adding to routing table\n");
		}

	} else if (type == DATA) {
		// DATA packet, forward towards root
		DATA_message_t* message = (DATA_message_t*) packetbuf_dataptr();
		forward_DATA(conn, message, &mote);

	} else if (type == OPEN) {
		// OPEN packet, forward towards destination
		OPEN_message_t* message = (OPEN_message_t*) packetbuf_dataptr();
		linkaddr_t dst_addr = message->dst_addr;
		if (linkaddr_cmp(&dst_addr, &(mote.addr))) {
			printf("Computation mote, no valve to open.\n");
		} else {
			forward_OPEN(conn, message, &mote);
		}

	} else {
		printf("Unknown runicast message received.\n");
	}

}

/**
 * Callback function, called when an unicast packet is sent
 */
void runicast_sent(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions) {

}

/**
 * Callback function, called when an unicast packet has timed out
 */
void runicast_timeout(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions) {
	printf("Runicast packet timed out.\n");
}

// Runicast callback functions
const struct runicast_callbacks runicast_callbacks = {runicast_recv, runicast_sent, runicast_timeout};



//////////////////////////////
///  BROADCAST CONNECTION  ///
//////////////////////////////

/**
 * Callback function, called when a broadcast packet is received
 */
void broadcast_recv(struct broadcast_conn *conn, const linkaddr_t *from) {

	// Strength of the last received packet
	signed char rss = cc2420_last_rssi;

	uint8_t* data = (uint8_t*) packetbuf_dataptr();
	uint8_t type = *data;

	if (type == DIS) { // DIS message received
		
		//printf("DIS packet received.\n");
		// If the mote is already in a DODAG, send DIO packet
		if (mote.in_dodag) {
			send_DIO(conn, &mote);
		}

	} else if (type == DIO) { // DIO message received

		//printf("DIO message received from %u.%u\n", from->u8[0], from->u8[1]);

		DIO_message_t* message = (DIO_message_t*) packetbuf_dataptr();
		if (linkaddr_cmp(from, &(mote.parent->addr))) { // DIO message received from parent

			if (message->rank == INFINITE_RANK) { // Parent has detached from the DODAG
				detach(&mote);
				reset_timers(&t_timer);
			} else { // Update info
				// Restart timer to delete lost parent
				ctimer_set(&parent_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
					parent_callback, NULL);
				if (update_parent(&mote, message->rank, rss)) {
					// Rank of parent has changed, reset trickle timer
					reset_timers(&t_timer);
				}
			}

		} else {
			// DIO message received from other mote
			uint8_t code = choose_parent(&mote, from, message->rank, rss);
		    if (code == PARENT_NEW) {
				reset_timers(&t_timer);
		    	send_DAO(&runicast, &mote);

		    	// Start all timers that are used when mote is in DODAG
		    	ctimer_set(&send_timer, trickle_random(&t_timer),
					send_callback, NULL);
				ctimer_set(&DAO_timer, trickle_random(&t_timer),
					DAO_callback, NULL);
				ctimer_set(&parent_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
					parent_callback, NULL);
				ctimer_set(&children_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
					children_callback, NULL);

		    } else if (code == PARENT_CHANGED) {
		    	// If parent has changed, send DIO message to update children
		    	// and DAO to update routing tables, then reset timers
		    	send_DIO(conn, &mote);
		    	send_DAO(&runicast, &mote);
		    	reset_timers(&t_timer);
		    }
		}

	} else { // Unknown message received
		printf("Unknown broadcast message received.\n");
	}

}

// Broadcast callback function
const struct broadcast_callbacks broadcast_call = {broadcast_recv};



//////////////////////
///  MAIN PROCESS  ///
//////////////////////

// Create and start the process
PROCESS(computation_mote, "Computation mote");
AUTOSTART_PROCESSES(&computation_mote);


PROCESS_THREAD(computation_mote, ev, data) {

	if (!created) {
		init_mote(&mote);
		trickle_init(&t_timer);
		created = 1;
	}

	PROCESS_EXITHANDLER(broadcast_close(&broadcast); runicast_close(&runicast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	runicast_open(&runicast, 144, &runicast_callbacks);

	while(1) {

		// Start the sending timer
		ctimer_set(&send_timer, trickle_random(&t_timer),
			send_callback, NULL);

		// Wait for the ctimers to trigger
		PROCESS_YIELD();

	}

	PROCESS_END();

}


/**
 * Code for the sensor nodes
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"

#include "computation.h"
#include "trickle-timer.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"

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
 * This function is called when there is a change in the network.
 */
void reset_timers() {
	trickle_reset(&t_timer);
	ctimer_set(&send_timer, trickle_random(&t_timer),
		send_callback, NULL);
	ctimer_set(&DAO_timer, trickle_random(&t_timer),
		DAO_callback, NULL);
}

/**
 * Resets the trickle timer, and stops all timers for events that happen when the mote is in the network.
 * This function is called when the mote detaches from the network.
 */
void stop_timers() {
	trickle_reset(&t_timer);
	ctimer_set(&send_timer, trickle_random(&t_timer),
		send_callback, NULL);
	ctimer_stop(&DAO_timer);
	ctimer_stop(&parent_timer);
	ctimer_stop(&children_timer);
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
		stop_timers();
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
		reset_timers();
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
				reset_timers();

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
		// DATA packet, compute if mote is in list or if there is room
		// Otherwise, forward towards root
		DATA_message_t* message = (DATA_message_t*) packetbuf_dataptr();
		int ret = add_and_check_valve(message->src_addr, computed_motes, message->data);
		if (ret == OPEN_VALVE) {
			// Send OPEN message to mote
			send_OPEN(conn, message->src_addr, &mote);
		} else if (ret == CANNOT_ADD_MOTE) {
			// No room to add child, forward towards root
			forward_DATA(conn, message, &mote);
		}

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
				stop_timers();
			} else { // Update info
				// Restart timer to delete lost parent
				ctimer_set(&parent_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
					parent_callback, NULL);
				if (update_parent(&mote, message->rank, rss)) {
					send_DIO(conn, &mote);
					// Rank of parent has changed, reset trickle timer
					reset_timers();
				}
			}

		} else {
			// DIO message received from other mote
			uint8_t code = choose_parent(&mote, from, message->rank, rss);
		    if (code == PARENT_NEW) {
				reset_timers();
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
		    	reset_timers();
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


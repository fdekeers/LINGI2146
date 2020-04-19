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

// Period of sending data messages
#define DATA_PERIOD 60


// Represents the attributes of this mote
mote_t mote;

// 1 if the mote has been created. Used to create the mote only once.
uint8_t created = 0;

// 1 if a message from the parent has been received in the timer interval.
uint8_t info_from_parent = 0;

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

// Callback timer to print routing table
struct ctimer print_timer;

// Callback timer to send data
struct ctimer data_timer;


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

/**
 * Callback function that will print the routing table.
 */
void print_callback(void *ptr) {
	// Reset the timer
	ctimer_reset(&print_timer);
	// Print the routing table
	hashmap_print(mote.routing_table);
}

/**
 * Callback function that will send a data message to the parent.
 */
void data_callback(void *ptr) {
	// Reset the timer
	ctimer_reset(&data_timer);

	// Send the data to parent if mote is in DODAG
	if (mote.in_dodag) {
		send_DATA(&runicast, &mote);
	}
}



/////////////////////////////
///  RUNICAST CONNECTION  ///
/////////////////////////////

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
		if (err >= 0) {

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
			// TODO : Open valve
		} else {
			forward_OPEN(conn, message, &mote, dst_addr);
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
				ctimer_set(&data_timer, CLOCK_SECOND*DATA_PERIOD,
					data_callback, NULL);

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

const struct broadcast_callbacks broadcast_call = {broadcast_recv};



//////////////////////
///  MAIN PROCESS  ///
//////////////////////

// Create and start the process
PROCESS(sensor_mote, "Sensor mote");
AUTOSTART_PROCESSES(&sensor_mote);


PROCESS_THREAD(sensor_mote, ev, data) {

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
		
		/*ctimer_set(&print_timer, CLOCK_SECOND*5 + random_rand() % CLOCK_SECOND,
			print_callback, NULL);*/

		// Wait for the ctimers to trigger
		PROCESS_YIELD();

	}

	PROCESS_END();

}

/**
 * Code for the sensor nodes
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"

#include "routing.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"


// Represents the attributes of this mote
mote_t mote;

// 1 if the mote has been created. Used to create the mote only once.
uint8_t created = 0;

// 1 if a message from the parent has been received in the timer interval.
uint8_t info_from_parent = 0;


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

		if (hashmap_put(mote.routing_table, child_addr, *from) == MAP_OK) {
			linkaddr_t next_hop;
			hashmap_get(mote.routing_table, child_addr, &next_hop);
			/*printf("Added child %u.%u. Reachable from %u.%u.\n",
				child_addr.u8[0], child_addr.u8[1],
				next_hop.u8[0], next_hop.u8[1]);*/
			forward_DAO(conn, &mote, child_addr);

			if (linkaddr_cmp(&child_addr, from)) {
				// linkaddr_cmp returns non-zero if addresses are equal

				// update timestamp of the child now or add the new child
				update_timestamp(&mote, clock_seconds(), child_addr);
			}
			
		} else {
			printf("Error adding to routing table\n");
		}

	} else if (type == DATA) {

		DATA_message_t* message = (DATA_message_t*) packetbuf_dataptr();
		forward_DATA(conn, message, &mote);

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

// Reliable unicast connection
const struct runicast_callbacks runicast_callbacks = {runicast_recv, runicast_sent, runicast_timeout};
static struct runicast_conn runicast;


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

		DIO_message_t* message = (DIO_message_t*) packetbuf_dataptr();
		if (linkaddr_cmp(from, &(mote.parent->addr))) { // DIO message received from parent

			if (message->rank == INFINITE_RANK) { // Parent has detached from the DODAG
				detach(&mote);
				send_DIO(conn, &mote);
			} else { // Update info
				info_from_parent = 1;
				mote.parent->rank = message->rank;
				mote.rank = message->rank + 1;
				mote.parent->rss = rss;
				send_DAO(&runicast, &mote);
			}

		} else {
			// DIO message received from other mote
			uint8_t code = choose_parent(&mote, from, message->rank, rss);
		    if (code == PARENT_INIT) {
		    	// If parent was initialized, send DAO message to new parent
		    	send_DAO(&runicast, &mote);
		    } else if (code == PARENT_CHANGED) {
		    	// If parent has changed, send DIO message to update children and DAO to update routing tables
		    	send_DIO(conn, &mote);
		    	send_DAO(&runicast, &mote);
		    }
		}

	} else { // Unknown message received
		printf("Unknown broadcast message received.\n");
	}

}

// Broadcast connection
const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;



/////////////////////////
///  CALLBACK TIMERS  ///
/////////////////////////

// Callback timer to send information messages
struct ctimer send_timer;

// Callback timer to delete parent or children
struct ctimer delete_timer;

// Callback timer to print routing table
struct ctimer print_timer;

// Callback timer to send data
struct ctimer data_timer;

/**
 * Callback function that will send the appropriate message when ctimer has expired.
 */
void send_callback(void *ptr) {
	// Reset the timer
	ctimer_reset(&send_timer);

	// Send the appropriate message
	if (!mote.in_dodag) {
		send_DIS(&broadcast);
	} else {
		send_DIO(&broadcast, &mote);
		send_DAO(&runicast, &mote);
	}

}

/**
 * Callback function that will delete the parent or the children in the routing table,
 * if no information from them has been received.
 */
void delete_callback(void *ptr) {
	// Reset the timer
	ctimer_reset(&delete_timer);

	// Delete parent if no information has been received since a long time
	if (!info_from_parent) {
		detach(&mote);
	}

	// Reset marker for information coming from parent
	info_from_parent = 0;

	// Delete children that haven't sent messages since a long time
	hashmap_delete_timeout(mote.routing_table, clock_seconds(), TIMEOUT_CHILD);
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



//////////////////////
///  MAIN PROCESS  ///
//////////////////////

// Create and start the process
PROCESS(sensor_mote, "Sensor mote");
AUTOSTART_PROCESSES(&sensor_mote);


PROCESS_THREAD(sensor_mote, ev, data) {

	if (!created) {
		init_mote(&mote);
		created = 1;
	}

	PROCESS_EXITHANDLER(broadcast_close(&broadcast); runicast_close(&runicast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	runicast_open(&runicast, 144, &runicast_callbacks);

	while(1) {

		ctimer_set(&send_timer, CLOCK_SECOND*SEND_PERIOD + random_rand() % CLOCK_SECOND,
			send_callback, NULL);

		ctimer_set(&delete_timer, CLOCK_SECOND*DELETE_PERIOD  + random_rand() % CLOCK_SECOND,
			delete_callback, NULL);

		ctimer_set(&print_timer, CLOCK_SECOND*5 + random_rand() % CLOCK_SECOND,
			print_callback, NULL);

		ctimer_set(&data_timer, CLOCK_SECOND*10,
			data_callback, NULL);

		// Wait for the ctimers to trigger
		PROCESS_YIELD();

	}

	PROCESS_END();

}

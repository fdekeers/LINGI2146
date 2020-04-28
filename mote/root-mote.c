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
#include "serial-line.h"


// Represents the attributes of this mote
mote_t mote;
uint8_t created = 0;

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

// Callback timer to delete parent or children
struct ctimer children_timer;

// Callback timer to print routing table
struct ctimer print_timer;

/**
 * Callback function that will send the appropriate message when ctimer has expired.
 */
void send_callback(void *ptr) {

	// Send a DIO message
	send_DIO(&broadcast, &mote);

	// Update the trickle timer
	trickle_update(&t_timer);

	// Restart the timer with a new random value
	ctimer_set(&send_timer, trickle_random(&t_timer), send_callback, NULL);

}

/**
 * Callback function that will delete unresponsive children from the routing table.
 */
void children_callback(void *ptr) {

	// Delete children that haven't sent messages since a long time
	if (hashmap_delete_timeout(mote.routing_table)) {
		// Children have been deleted, reset trickle timer
		trickle_reset(&t_timer);
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
 * Resets the trickle timer and restarts the callback timer that uses it (sending timer)
 */
void reset_timers(trickle_timer_t *timer) {
	trickle_reset(&t_timer);
	ctimer_set(&send_timer, trickle_random(&t_timer),
		send_callback, NULL);
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
		if (err == MAP_NEW) { // A new child was added to the routing table
			// Reset trickle timer and sending timer
			reset_timers(&t_timer);

			/*if (linkaddr_cmp(&child_addr, from)) {
				// linkaddr_cmp returns non-zero if addresses are equal

				// update timestamp of the child now or add the new child
				unsigned long time = clock_seconds();
				update_timestamp(&mote, time, child_addr);
			}*/

		} else if (err < 0) {
			printf("Error adding to routing table\n");
		}

	} else if (type == DATA) {

		DATA_message_t* message = (DATA_message_t*) packetbuf_dataptr();
		printf("%u/%u/%u\n", message->type, message->src_addr, message->data);

	} else {
		printf("Unknown runicast message received.\n");
	}


}

void runicast_sent(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions) {

}

void runicast_timeout(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions) {

}

const struct runicast_callbacks runicast_callbacks = {runicast_recv, runicast_sent, runicast_timeout};


//////////////////////////////
///  BROADCAST CONNECTION  ///
//////////////////////////////

/**
 * Callback function, called when a broadcast packet is received
 */
void broadcast_recv(struct broadcast_conn *conn, const linkaddr_t *from) {

	uint8_t* data = (uint8_t*) packetbuf_dataptr();
	uint8_t type = *data;

	if (type == DIS) {
		//printf("DIS packet received.\n");
		// If the mote is already in a DODAG, send DIO packet
		if (mote.in_dodag) {
			send_DIO(conn, &mote);
		}
	} else if (type == DIO) {
		//printf("DIO message received.\n");
	} else {
		printf("Unknown broadcast message received.\n");
	}

}

const struct broadcast_callbacks broadcast_call = {broadcast_recv};



//////////////////////
///  MAIN PROCESS  ///
//////////////////////

// Create and start the process
PROCESS(root_mote, "Root mote");
PROCESS(server_communication, "Server communication");

AUTOSTART_PROCESSES(&root_mote, &server_communication);

PROCESS_THREAD(root_mote, ev, data) {

	if (!created) {
		init_root(&mote);
		trickle_init(&t_timer);
		created = 1;
	}

	PROCESS_EXITHANDLER(broadcast_close(&broadcast); runicast_close(&runicast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	runicast_open(&runicast, 144, &runicast_callbacks);

	while(1) {

		// Start all the timers
		ctimer_set(&send_timer, trickle_random(&t_timer),
			send_callback, NULL);
		ctimer_set(&children_timer, CLOCK_SECOND*TIMEOUT - random_rand() % (CLOCK_SECOND*5),
			children_callback, NULL);
		/*ctimer_set(&print_timer, CLOCK_SECOND*5 + random_rand() % CLOCK_SECOND,
			print_callback, NULL);*/

		// Wait for the ctimer to trigger
		PROCESS_YIELD();

	}

	PROCESS_END();
}

PROCESS_THREAD(server_communication, ev, data) {
    PROCESS_BEGIN();

    while(1) {
        PROCESS_YIELD();
        if(ev == serial_line_event_message) {
            char *strData = (char *)data;
            printf("Received line: %s\n", strData);

            printf("Type: %c", strData[0]);

        }
    }
    PROCESS_END();
}

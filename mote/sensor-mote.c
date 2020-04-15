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
uint8_t created = 0;


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

		printf("DAO message received from %d.%d\n", from->u8[0], from->u8[1]);

		DAO_message_t* message = (DAO_message_t*) packetbuf_dataptr();

		// Address of the mote that sent the DAO packet
		linkaddr_t child_addr = message->src_addr;

		printf("Child address : %d.%d\n", child_addr.u8[0], child_addr.u8[1]);

		if (hashmap_put(mote.routing_table, child_addr, *from) == MAP_OK) {
			linkaddr_t *next_hop = (linkaddr_t*) malloc(sizeof(linkaddr_t));
			hashmap_get(mote.routing_table, child_addr, next_hop);
			printf("Added child %d.%d. Reachable from %d.%d.\n",
				child_addr.u8[0], child_addr.u8[1],
				next_hop->u8[0], next_hop->u8[1]);
			forward_DAO(conn, &mote, child_addr);

			if (linkaddr_cmp(&child_addr, from)) {
				// linkaddr_cmp returns non-zero if addresses are equal

				// update timestamp of the child now or add the new child
				update_timestamp(&mote, clock_seconds(), child_addr);
			}
			
			free(next_hop);
		} else {
			printf("Error adding to routing table\n");
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

	uint8_t* data = (uint8_t*) packetbuf_dataptr();
	uint8_t type = *data;

	// Strength of the last received packet
	signed char rss = cc2420_last_rssi - 45;

	if (type == DIS) { // DIS message received
		
		printf("DIS packet received.\n");
		// If the mote is already in a DODAG, send DIO packet
		if (mote.in_dodag) {
			send_DIO(conn, &mote);
		}
	} else if (type == DIO) { // DIO message received

		DIO_message_t* message = (DIO_message_t*) packetbuf_dataptr();
		if (linkaddr_cmp(from, &(mote.parent->addr))) {
			// DIO message received from parent, update parent info
			mote.parent->rank = message->rank;
			mote.parent->rss = rss;
		} else if (choose_parent(&mote, from, message->rank, rss)) {
		    send_DAO(&runicast, &mote);
		}

	} else { // Unknown message received
		printf("Unknown broadcast message received.\n");
	}

}

// Broadcast connection
const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;


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

	static struct etimer timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	runicast_open(&runicast, 144, &runicast_callbacks);

	while(1) {

		etimer_set(&timer, CLOCK_SECOND*2 + random_rand() % CLOCK_SECOND);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		if (!mote.in_dodag) {
			send_DIS(&broadcast);
		} else {
			send_DIO(&broadcast, &mote);
		}

	}

	PROCESS_END();

}

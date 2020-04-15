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

		if (hashmap_put(mote.routing_table, child_addr, *from) == MAP_OK) {
			linkaddr_t *next_hop = (linkaddr_t*) malloc(sizeof(linkaddr_t));
			hashmap_get(mote.routing_table, child_addr, next_hop); 
			printf("Added child %d.%d. Reachable from %d.%d.\n",
				child_addr.u8[0], child_addr.u8[1],
				next_hop->u8[0], next_hop->u8[1]);
			forward_DAO(conn, &mote, child_addr);
		} else {
			printf("Error adding to routing table\n");
		}

	} else {
		printf("Received unknown unicast message\n");
	}


}

void runicast_sent(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions) {

}

void runicast_timeout(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions) {

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

	signed char rss = cc2420_last_rssi - 45;

	if (type == DIS) {
		printf("DIS packet received.\n");
		// If the mote is already in a DODAG, send DIO packet
		if (mote.in_dodag) {
			send_DIO(conn, &mote);
		}
	} else if (type == DIO) {
		printf("DIO message received.\n");
	} else {
		printf("Received message type unknown.\n");
	}

}

// Broadcast connection
const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;


//////////////////////
///  MAIN PROCESS  ///
//////////////////////

// Create and start the process
PROCESS(root_mote, "Root mote");
AUTOSTART_PROCESSES(&root_mote);


PROCESS_THREAD(root_mote, ev, data) {

	if (!created) {
		init_root(&mote);
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

		send_DIO(&broadcast, &mote);

	}

	PROCESS_END();

}
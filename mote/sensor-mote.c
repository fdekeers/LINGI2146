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

		// Address of the child, put in the DAO packet
		linkaddr_t* child_addr = (linkaddr_t*) malloc(sizeof(uint8_t)*LINKADDR_SIZE);
		child_addr->u8[0] = from->u8[0];
		child_addr->u8[1] = from->u8[1];
		mote.child_addr = child_addr;

		printf("Child set to %d.%d\n", mote.child_addr->u8[0], mote.child_addr->u8[1]);

		send_DAO(conn, &mote);

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

	// Strength of the last received packet
	signed char rss = cc2420_last_rssi - 45;

	if (type == DIS) {
		printf("DIS packet received.\n");
		// If the mote is already in a DODAG, send DIO packet
		if (mote.in_dodag) {
			send_DIO(conn, &mote);
		}
	} else if (type == DIO) {
		uint8_t rank_recv = *(data+1);
		if (choose_parent(&mote, from, rank_recv, rss))
		    send_DAO(&runicast, &mote);
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

		etimer_set(&timer, CLOCK_SECOND*2);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		if (!mote.in_dodag) {
			send_DIS(&broadcast);
		} else {
			printf("Already in DODAG.\n");
		}

	}

	PROCESS_END();

}
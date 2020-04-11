/**
 * Code for the sensor nodes
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/cc2420/cc2420.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"


//////////////////
//  DATA TYPES  //
//////////////////

typedef struct parent_mote {
	linkaddr_t* addr;
	uint8_t rank;
	signed char rss;
} parent_t;



//////////////////////////
//  VALUES & VARIABLES  //
//////////////////////////

// Values for the different types of RPL control messages
const uint8_t DIS = 0;
const uint8_t DIO = 1;
const uint8_t DAO = 2;

uint8_t in_dodag = 0;
uint8_t rank;
parent_t parent;



/////////////////
//  FUNCTIONS  //
/////////////////

/**
 * Broadcasts a DIS message.
 */
void send_DIS(struct broadcast_conn *conn) {

	size_t size = sizeof(uint8_t);

	uint8_t* data = (uint8_t*) malloc(size);
	*data = DIS;

	packetbuf_copyfrom((void*) data, size);
	broadcast_send(conn);
	printf("DIS packet broadcasted.\n");

}

/**
 * Broadcasts a DIO message, containing the rank of the node.
 */
void send_DIO(struct broadcast_conn *conn) {

	size_t size = sizeof(uint8_t);

	uint8_t* data = (uint8_t*) malloc(size);
	*data = DIO;
	*(data+1) = rank;

	packetbuf_copyfrom((void*) data, size*2);
	broadcast_send(conn);
	printf("DIO packet broadcasted, rank = %d\n", rank);

}

/**
 * Called when a DIS packet is received.
 */
void receive_DIS(struct broadcast_conn *conn) {
	printf("DIS packet received.\n");

	// If the node is already in a DODAG, send DIO packet
	if (in_dodag) {
		send_DIO(conn);
	}

}

/**
 * Selects the parent
 */
void choose_parent(const linkaddr_t* parent_addr, uint8_t parent_rank, signed char rss) {
	if (!in_dodag) {
		linkaddr_copy(parent.addr, parent_addr);
		parent.rank = parent_rank;
		parent.rss = rss;
		in_dodag = 1;
		printf("Parent set : Addr = %d.%d; Rank = %d\n",
			parent.addr->u8[0], parent.addr->u8[1], parent.rank);
	}
	else {
		printf("Already has a parent !\n");
	}
}

/**
 * Called when a DAO packet is received.
 */
void receive_DAO() {
	printf("DAO packet received.\n");
}


// Callback function, called when a broadcast packet is received
static void broadcast_recv(struct broadcast_conn *conn, const linkaddr_t *from) {

	uint8_t* data = (uint8_t*) packetbuf_dataptr();
	uint8_t type = *data;

	signed char rss = cc2420_last_rssi - 45;

	if (type == DIS) {
		receive_DIS(conn);
	} else if (type == DIO) {
		uint8_t rank_recv = *(data+1);
		choose_parent(from, rank_recv, rss);
	} else if (type == DAO) {
		receive_DAO();
	} else {
		printf("Received message type unknown.\n");
	}

}


////////////////////
//  MAIN PROCESS  //
////////////////////

// Broadcast connection
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

// Create and start the process
PROCESS(sensor_mote, "Sensor mote");
AUTOSTART_PROCESSES(&sensor_mote);


PROCESS_THREAD(sensor_mote, ev, data) {

	parent.addr = (linkaddr_t*) malloc(sizeof(uint8_t)*LINKADDR_SIZE);

	static struct etimer timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		etimer_set(&timer, CLOCK_SECOND*2);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		send_DIS(&broadcast);

	}

	PROCESS_END();

}
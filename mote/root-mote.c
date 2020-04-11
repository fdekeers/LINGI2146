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



//////////////////////////
//  VALUES & VARIABLES  //
//////////////////////////

// Values for the different types of RPL control messages
const uint8_t DIS = 0;
const uint8_t DIO = 1;
const uint8_t DAO = 2;

uint8_t rank = 0;



/////////////////
//  FUNCTIONS  //
/////////////////

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
	send_DIO(conn);

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

	if (type == DIS) {
		receive_DIS(conn);
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

	static struct etimer timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		etimer_set(&timer, CLOCK_SECOND*2);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		send_DIO(&broadcast);

	}

	PROCESS_END();

}
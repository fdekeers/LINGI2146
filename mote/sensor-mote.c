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


// Create and start the process
PROCESS(sensor_mote, "Sensor node");
AUTOSTART_PROCESSES(&sensor_mote);


// Callback function, called when a broadcast packet is received
static void broadcast_recv(struct broadcast_conn *conn, const linkaddr_t *from) {

	uint8_t* data = (uint8_t*) packetbuf_dataptr();
	uint8_t type = *data;

	if (type == DIS) {
		receive_DIS();
	} else if (type == DIO) {
		receive_DIO();
	} else if (type == DAO) {
		receive_DAO();
	} else {
		printf("Received message type unknown.\n");
	}

}

// Broadcast Rime connexion
static struct broadcast_conn broadcast;
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};


PROCESS_THREAD(sensor_mote, ev, data) {

	static struct etimer timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		etimer_set(&timer, CLOCK_SECOND*10);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		send_DIS(&broadcast);

	}

	PROCESS_END();

}
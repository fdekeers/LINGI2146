/**
 * Code for the sensor nodes
 */

#include "contiki.h"
#include "net/rime/rime.h"

#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
#include "random.h"


// Create and start the process
PROCESS(sensor_node, "Sensor node");
AUTOSTART_PROCESSES(&sensor_node);

// Broadcast Rime connexion
static struct broadcast_conn broadcast;
static const struct broadcast_callbacks broadcast_call = {};

PROCESS_THREAD(sensor_node, ev, data) {

	static struct etimer timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		etimer_set(&timer, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		printf("Test OK");

	}

	PROCESS_END();

}
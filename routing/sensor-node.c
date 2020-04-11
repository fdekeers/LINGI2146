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

PROCESS_THREAD(sensor_node, event, data) {

}
/**
 * Defines data and function for the computation motes.
 */

#include "routing.h"


///////////////////
///  CONSTANTS  ///
///////////////////

#define COMPUTED_BUFFER_FULL -1
#define OPEN_VALVE 1
#define CLOSE_VALVE 2
#define CANNOT_ADD_MOTE 3
#define MIN_NB_VALUES_COMPUTE 10 // minimum values needed to do the computation
#define MAX_NB_VALUES 30 // maximum number of values about the mote
#define MAX_NB_COMPUTED 5 // this node can compute the needed values for only this number of nodes
#define SLOPE_THRESHOLD 30 // definition of the threshold (in %) for which we should open valves to improve air quality



////////////////////
///  DATA TYPES  ///
////////////////////

typedef struct computed_mote {
	linkaddr_t addr;
	unsigned long timestamp;
	uint8_t in_use;
	uint16_t values[MAX_NB_VALUES];
	uint8_t first_value_index;
	uint8_t first_free_value_index;
	//size is not needed if when created, we add an element directly
	// using this, if the 2 indexes are equal, it will mean that the buffer is full-
} computed_mote_t;



///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Finds the index where the mote is located or could be added or COMPUTED_BUFFER_FULL if
 * the mote wasn't present and the buffer is full. It also deletes nodes that timed out
 */
int indexFind(linkaddr_t addr, computed_mote_t computed_motes[], unsigned long curr_time);

/**
 * Returns the computed slope. If first_free_value_index and first_value_index are equal,
 * we consider that the buffer is of the maximum size. This function shouldn't be called
 * on an empty buffer
 */
int slope_value(int index_mote, computed_mote_t computed_motes[]);

/**
 * Adds the information received from the mote and returns whether the valve should be opened or not
 */
int add_and_check_valve(linkaddr_t addr, computed_mote_t computed_motes[], uint16_t quality_air_value);

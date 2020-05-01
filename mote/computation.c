/**
 * Defines data and function for the computation motes.
 */

#include "computation.h"


///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Finds the index where the mote is located or could be added or COMPUTED_BUFFER_FULL if
 * the mote wasn't present and the buffer is full. It also deletes nodes that timed out
 */
int indexFind(linkaddr_t addr, computed_mote_t computed_motes[], unsigned long curr_time) {
	int index = COMPUTED_BUFFER_FULL;
	int i;
	for (i = 0; i < MAX_NB_COMPUTED; i++) {
		if (!computed_motes[i].in_use && index == COMPUTED_BUFFER_FULL) {
			index = i;
		}
		else if (linkaddr_cmp(&addr, &(computed_motes[i].addr))) {
			return i;
		}
		if (computed_motes[i].in_use && curr_time > computed_motes[i].timestamp+TIMEOUT) {
			// remove nodes that timed out
			computed_motes[i].in_use = 0;
			if (index == COMPUTED_BUFFER_FULL) {
				// this is the first unused node
				index = i;
			}
		}
	}
	return index;
}

/**
 * Returns the computed slope. If first_free_value_index and first_value_index are equal,
 * we consider that the buffer is of the maximum size. This function shouldn't be called
 * on an empty buffer
 */
int slope_value(int index_mote, computed_mote_t computed_motes[]) {
	computed_mote_t *elem = &(computed_motes[index_mote]);
	int nb_values = ((elem->first_free_value_index+MAX_NB_VALUES) - elem->first_value_index) % MAX_NB_VALUES;
	if (nb_values == 0)
		nb_values = MAX_NB_VALUES; // happens when pointers are the same
	int x[nb_values];
	uint16_t *y = (elem->values);
	int i;
	double sum_x = 0.0, sum_y = 0.0, sum_xx = 0.0, sum_xy = 0.0;
	for (i = 0; i < nb_values; i++) {
		x[i] = i;
		sum_x += i;
		double y_i = (double) (y[(elem->first_value_index+i)%MAX_NB_VALUES]);
		// casting to double is not a problem since value <= 500 and value is uint16_t (so no problem, even with the sign)
		sum_y += y_i;
		sum_xy = x[i]*y_i;
		sum_xx = x[i]*x[i];
	}
	double slope = (sum_x * sum_y - nb_values * sum_xy) / (sum_x * sum_x - nb_values * sum_xx);
	return ((int)(slope*100)) / 100;
}

/**
 * Adds the information received from the mote and returns whether the valve should be opened or not
 */
int add_and_check_valve(linkaddr_t addr, computed_mote_t computed_motes[], uint16_t quality_air_value) {
	unsigned long time = clock_seconds();
	int index_mote = indexFind(addr, computed_motes, time);
	if (index_mote == COMPUTED_BUFFER_FULL) {
		printf("Couldn't add mote %u.%u in the computation buffer\n", addr.u8[0], addr.u8[1]);
		return CANNOT_ADD_MOTE;
	}
	uint8_t enough_values = 0; // false
	computed_mote_t *elem = &(computed_motes[index_mote]);
	if (elem->in_use) {
		if (elem->first_value_index == elem->first_free_value_index) {
			// buffer is full
			enough_values = 1;
			elem->first_value_index = (elem->first_value_index + 1) % MAX_NB_VALUES; // increment so that this will be the next deleted value
		}
	} else {
		// adding a new node !
		elem->in_use = 1;
		elem->addr = addr;
		elem->first_value_index = 0;
		elem->first_free_value_index = 0;
		// we can add the value
	}
	elem->timestamp = time;
	(elem->values)[elem->first_free_value_index] = quality_air_value;
	elem->first_free_value_index = (elem->first_free_value_index + 1) % MAX_NB_VALUES;

	printf("Added mote %u.%u in the computation buffer\n", addr.u8[0], addr.u8[1]);

	if (((elem->first_free_value_index+MAX_NB_VALUES) - elem->first_value_index) % MAX_NB_VALUES > MIN_NB_VALUES_COMPUTE) {
		// enough values to compute whether we should open the valve
		enough_values = 1;
	}
	if (enough_values && slope_value(index_mote, computed_motes) >= SLOPE_THRESHOLD) {
		// >= since higher values are worse than lower values
		return OPEN_VALVE;
	}
	return CLOSE_VALVE;
}

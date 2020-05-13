/**
 * Trickle timer for the sending of periodic control messages.
 */

#include "trickle-timer.h"


///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Initializes a trickle timer, by setting its attributes to the default.
 */
void trickle_init(trickle_timer_t* timer) {
	timer->T = (uint8_t) T_MIN;
}

/**
 * Returns a random duration between T/2 and T.
 */
uint16_t trickle_random(trickle_timer_t* timer) {
	uint16_t min = CLOCK_SECOND * (timer->T)/2;
	uint16_t max = CLOCK_SECOND * timer->T;
	uint16_t random_delay = random_rand() % (max-min + 1) + min;
	return random_delay;
}

/**
 * Updates the value of T, by doubling it (up to T_MAX).
 */
void trickle_update(trickle_timer_t* timer) {
	uint8_t new_T = timer->T * 2;
	if (new_T > T_MAX) {
		timer->T = (uint8_t) T_MAX;
	} else {
		timer->T = new_T;
	}
}

/**
 * Resets the timer, by setting T to T_MIN, and c to 0.
 */
void trickle_reset(trickle_timer_t* timer) {
	timer->T = (uint8_t) T_MIN;
}

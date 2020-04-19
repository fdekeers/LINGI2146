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
	timer->T = T_MIN;
	timer->c = 0;
	timer->k = K;
}

/**
 * Returns a random float between a and b.
 */
float random_float(float a, float b) {
	float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

/**
 * Returns a random duration between T/2 and T.
 */
float random_delay(trickle_timer_t* timer) {
	srand(time(NULL));
	uint8_t T = timer->T;
	return random_float(T / 2.0, (float) T);
}

/**
 * Updates the value of T, by doubling it (up to T_MAX).
 */
void update_T(trickle_timer_t* timer) {
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
	timer->T = T_MIN;
	timer->c = 0;
}

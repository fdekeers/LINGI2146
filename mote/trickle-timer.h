/**
 * Trickle timer for the sending of periodic control messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>



///////////////////
///  CONSTANTS  ///
///////////////////

#define T_MIN  2
#define T_MAX  30
#define K      4



////////////////////
///  DATA TYPES  ///
////////////////////

typedef struct trickle_timer {
	uint8_t T;
	uint8_t c;
	uint8_t k;
} trickle_timer_t;



///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Initializes a trickle timer, by setting its attributes to the default.
 */
void trickle_init(trickle_timer_t* timer);

/**
 * Returns a random float between a and b.
 */
float random_float(float a, float b);

/**
 * Returns a random duration between T/2 and T.
 */
float random_delay(trickle_timer_t* timer);

/**
 * Updates the value of T, by doubling it (up to T_MAX)
 */
void update_T(trickle_timer_t* timer);

/**
 * Resets the timer, by setting T to T_MIN, and c to 0.
 */
void trickle_reset(trickle_timer_t* timer);
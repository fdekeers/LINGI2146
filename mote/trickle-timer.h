/**
 * Trickle timer for the sending of periodic control messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "contiki.h"
#include "random.h"


///////////////////
///  CONSTANTS  ///
///////////////////

#define T_MIN  2
#define T_MAX  20



////////////////////
///  DATA TYPES  ///
////////////////////

typedef struct trickle_timer {
	uint8_t T;
} trickle_timer_t;



///////////////////
///  FUNCTIONS  ///
///////////////////

/**
 * Initializes a trickle timer, by setting its attributes to the default.
 */
void trickle_init(trickle_timer_t* timer);

/**
 * Returns a random duration between T/2 and T.
 */
uint16_t trickle_random(trickle_timer_t* timer);

/**
 * Updates the value of T, by doubling it (up to T_MAX)
 */
void trickle_update(trickle_timer_t* timer);

/**
 * Resets the timer, by setting T to T_MIN, and c to 0.
 */
void trickle_reset(trickle_timer_t* timer);
/*
 * Adaptation by : Alexandre Gobeaux, François De Keersmaeker, Nicolas van de Walle
 * 
 * This is a hashmap from uint16_t to linkaddr_t. We adapted it from the string
 * generic hashmap cited below.
 * It is permitted to use linkaddr_t instead of uint16_t. However, we assume that those
 * will consist of an array of 2 unsigned chars. (LINKADDR_SIZE = 2) 
 *
 * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/
 *
 * Modified by Pete Warden to fix a serious performance problem, support strings as keys
 * and removed thread synchronization - http://petewarden.typepad.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "contiki.h"
#include "net/rime/rime.h"

/* ======================
 *  CONSTANTS DEFINITION
 * ====================== */

#define MAP_MISSING -3		/* No such element */
#define MAP_FULL -2		/* Hashmap is full, should NOT be >= 0 */
#define MAP_OMEM -1		/* Out of Memory */
#define MAP_OK 0		/* OK */
#define MAP_NEW 1		/* The added element is new */
#define MAP_UPDATE 2		/* The added element was already in the map */

#define INITIAL_SIZE (16)	// initial size of hashmap
#define MAX_CHAIN_LENGTH (7)	// number of "looks after" for linear probing

// Timeout [sec] to know when to forget a child
#define TIMEOUT 100

/* Debug mode, enable debug printf */
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

/* =======================
 *  STRUCTURES DEFINITION
 * ======================= */

/** We need to keep keys and values
 * the key should be the node from which we received a message
 * the data should be the next-hop to get to the key node
 */
typedef struct _hashmap_element{
	uint16_t key;
	uint8_t in_use;
	linkaddr_t data;
	unsigned long time;
} hashmap_element;

/** A hashmap has some maximum size and current size,
 * as well as the data to hold
 */
typedef struct _hashmap_map{
	int table_size;
	int size;
	hashmap_element *data;
} hashmap_map;

/* ============================
 *  INNER FUNCTIONS DEFINITION 
 * ============================ */

/**
 * Converts a linkaddr_t to a uint16_t
 */
uint16_t linkaddr2uint16_t (linkaddr_t x);

/**
 * Calloc reimplemented based on malloc and memset
 * Returns a pointer to an allocated memory of size nmemb*size
 * 		or NULL if it failed
 */
void *my_calloc(int nmemb, int size);

/**
 * Returns the index of the location in data of
 * the element that can be used to store information about
 * the given key or MAP_FULL if too many probing is present
 */
int hashmap_hash(hashmap_map *m, uint16_t key);

/**
 * Fills the new_array of data based on the old_array and modifies the different
 * elements of the hashmap m accordingly.
 * Return value : MAP_OK if everything was fine, MAP_OMEM if there is a bug that
 * 		  should not even be there, or MAP_FULL if we have to rehash
 */
int hashmap_fill_rehash(hashmap_map *m, hashmap_element *old_array, int old_table_size, hashmap_element *new_array, int new_table_size);

/**
 * Changes the size of the hashmap (at least the double + 1, more if multiple
 * rehashes have to be made) and rehashes all the elements.
 */
int hashmap_rehash(hashmap_map *m);

/* =============================
 *  EXTERN FUNCTIONS DEFINITION
 * ============================= */

/**
 * Returns an empty hashmap, or NULL on failure
 */
extern hashmap_map *hashmap_new();

/**
 * Adds/updates a pointer to the hashmap with some key
 * If the element was already present, the data is overwritten with the new one
 * Return value : MAP_OMEM if out of memory, MAP_FULL if rehash has to be called
 *		  while already called by rehash, MAP_NEW if an element was added,
 * 		  MAP_UPDATE if an element was updated.
 */
extern int hashmap_put_int(hashmap_map *m, uint16_t key, linkaddr_t value, unsigned long time, uint8_t isRehashing);

/**
 * $arg will point to the element with the given key
 * Return value : MAP_OK if a value with the given key exists, MAP_MISSING otherwise
 */
extern int hashmap_get_int(hashmap_map *m, uint16_t key, linkaddr_t *arg);

/**
 * Removes an element with that key from the map
 */
extern int hashmap_remove_int(hashmap_map *m, uint16_t key);

/**
 * The following functions are calling the upper ones with slightly modified arguments
 * These function are easier to call
 */
extern int hashmap_put(hashmap_map *m, linkaddr_t key, linkaddr_t value);
extern int hashmap_get(hashmap_map *m, linkaddr_t key, linkaddr_t *arg);
extern int hashmap_remove(hashmap_map *m, linkaddr_t key);

/**
 * Deallocates the hashmap
 */
extern void hashmap_free(hashmap_map *m);

/**
 * Returns the length of the hashmap (0 in the null case)
 */
extern int hashmap_length(hashmap_map *m);

/**
 * Prints the content of the hashmap
 */
extern void hashmap_print(hashmap_map *m);

/**
 * Removes entries that have timed out (based on arguments current_time and timeout_delay)
 * Design choice : unsigned long overflow is not taken into account since it would wrap up in ~= 135 years
 * Returns 1 if at least one element has been removed, 0 if no element has been removed.
 */
extern int hashmap_delete_timeout(hashmap_map *m);

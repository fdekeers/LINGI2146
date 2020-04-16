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

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

/* We need to keep keys and values */
typedef struct _hashmap_element{
	uint16_t key;
	uint8_t in_use;
	linkaddr_t data;
	unsigned long time;
} hashmap_element;

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _hashmap_map{
	int table_size;
	int size;
	hashmap_element *data;
} hashmap_map;

extern hashmap_map * hashmap_new();
extern int hashmap_put_int(hashmap_map *m, uint16_t key, linkaddr_t value, unsigned long time);
extern int hashmap_get_int(hashmap_map *m, uint16_t key, linkaddr_t *arg);
extern int hashmap_remove_int(hashmap_map *m, uint16_t key);
extern int hashmap_put(hashmap_map *m, linkaddr_t key, linkaddr_t value);
extern int hashmap_get(hashmap_map *m, linkaddr_t key, linkaddr_t *arg);
extern int hashmap_remove(hashmap_map *m, linkaddr_t key);
extern uint16_t linkaddr2uint16_t (linkaddr_t x);
extern void hashmap_free(hashmap_map *m);
extern int hashmap_length(hashmap_map *m);

/**
 * Prints the content of the hashmap.
 */
extern void hashmap_print(hashmap_map *m);

/**
 * Removes entries that have timeout (based on arguments current_time and timeout_delay)
 */
void hashmap_delete_timeout(hashmap_map *m, unsigned long current_time, unsigned long timeout_delay);


#define INITIAL_SIZE (16) // initial size of hashmap
#define MAX_CHAIN_LENGTH (8) // number of "looks after" for linear probing

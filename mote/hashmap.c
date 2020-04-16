#include "hashmap.h"

/**
 * Converts a linkaddr_t to a uint16_t
 */
uint16_t linkaddr2uint16_t (linkaddr_t x) {
    uint8_t a = x.u8[0]; uint8_t b = x.u8[1];
    uint16_t newval = (((uint16_t) a) << 8 ) | ((uint16_t) b);
    return newval;
}

/**
 * Calloc reimplemented based on malloc and memset
 * Returns a pointer to an allocated memory of size nmemb*size
 * 		or NULL if it failed
 */
void *my_calloc(int nmemb, int size) {
	void *malloced = malloc(nmemb*size);
	if (!malloced) {
		return NULL;
	}
	memset(malloced, 0, nmemb*size);
	return malloced;
}

/**
 * Returns the index of the location in data of
 * the element that can be used to store information about
 * the given key or MAP_FULL if too many probing is present
 */
int hashmap_hash(hashmap_map *m, uint16_t key) {
	int curr;
	int i;

	/* If full, return immediately */
	if(m->size >= (m->table_size/2)) return MAP_FULL;

	/* Find the best index */
	curr = key % m->table_size;

	/* Value of index if found */
	int firstInd = MAP_FULL;

	/* Linear probing */
	for(i = 0; i< MAX_CHAIN_LENGTH; i++) {
		if(m->data[curr].in_use == 0) {
			if (firstInd == -1) {
				firstInd = curr;
			}
		}

		if(m->data[curr].in_use == 1 && (m->data[curr].key == key)) {
			if (firstInd != -1) {
				// better to move closer !
				memcpy((m->data)+firstInd, (m->data)+curr, sizeof(hashmap_element));
				m->data[curr].in_use = 0;
				return firstInd;
			}
			return curr;
		}
		curr = (curr + 1) % m->table_size;
	}

	return firstInd; // returns MAP_FULL if not found
}



/**
 * Doubles the size of the hashmap, and rehashes all the elements
 */
int hashmap_rehash(hashmap_map *m) {
	int i;
	int old_size;
	hashmap_element* curr;

	/* Setup the new elements */
	hashmap_element* temp = (hashmap_element *)
		my_calloc(2 * m->table_size, sizeof(hashmap_element));
	if(!temp) return MAP_OMEM;

	/* Update the array */
	curr = m->data;
	m->data = temp;

	/* Update the size */
	old_size = m->table_size;
	m->table_size = 2 * m->table_size;
	m->size = 0;

	/* Rehash the elements */
	for(i = 0; i < old_size; i++)
	{
        int status;
        if (curr[i].in_use == 0)
            continue;
            
		status = hashmap_put_int(m, curr[i].key, curr[i].data, curr[i].time);
		if (status != MAP_OK)
			return status;
	}

	free(curr);

	return MAP_OK;
}

/**
 * Returns an empty hashmap, or NULL on failure
 */
hashmap_map * hashmap_new() {
	hashmap_map *m = (hashmap_map*) malloc(sizeof(hashmap_map));
	if(!m) goto err;

	m->data = (hashmap_element*) my_calloc (INITIAL_SIZE, sizeof(hashmap_element));
	if(!m->data) goto err;

	m->table_size = INITIAL_SIZE;
	m->size = 0;

	return m;
	err:
		if (m)
			hashmap_free(m);
		return NULL;
}

/**
 * Adds/updates a pointer to the hashmap with some key
 * If the element was already present, the data is overwritten with the new one
 * Return value : MAP_OMEM if out of memory, MAP_OK otherwise
 */
int hashmap_put_int(hashmap_map *m, uint16_t key, linkaddr_t value, unsigned long time) {
	int index;

	/* Find a place to put our value */
	index = hashmap_hash(m, key);
	while(index == MAP_FULL) {
		if (hashmap_rehash(m) == MAP_OMEM) {
			return MAP_OMEM;
		}
		index = hashmap_hash(m, key);
	}

	/* Set the data */
	m->data[index].data = value;
	m->data[index].time = time;
	m->data[index].key = key;
	m->data[index].in_use = 1;
	m->size++; 

	return MAP_OK;
}

/**
 * Adds a pointer to the hashmap with some key
 * If the element was already present, the data is overwritten with the new one
 * Return value : MAP_OMEM if out of memory, MAP_OK otherwise
 */
int hashmap_put(hashmap_map *m, linkaddr_t key, linkaddr_t value) {
	unsigned long time = clock_seconds();
    return hashmap_put_int(m, linkaddr2uint16_t(key), value, time);
}

/**
 * $arg will point to the element with the given key
 * Return value : MAP_OK if a value with the given key exists, MAP_MISSING otherwise
 */
int hashmap_get_int(hashmap_map *m, uint16_t key, linkaddr_t *arg) {
	int curr;
	int i;

	/* Find data location */
	curr = hashmap_hash(m, key);

	/* Linear probing, if necessary */
	for(i = 0; i<MAX_CHAIN_LENGTH; i++) {

        uint8_t in_use = m->data[curr].in_use;
        if (in_use == 1) {
            if (m->data[curr].key == key) {
                *arg = (m->data[curr].data);
                return MAP_OK;
            }
		}

		curr = (curr + 1) % m->table_size;
	}

	//*arg = NULL;

	/* Not found */
	return MAP_MISSING;
}

/**
 * $arg will point to the element with the given key
 * Return value : MAP_OK if a value with the given key exists, MAP_MISSING otherwise
 */
int hashmap_get(hashmap_map *m, linkaddr_t key, linkaddr_t *arg) {
    return hashmap_get_int(m, linkaddr2uint16_t(key), arg);
}

/**
 * Removes an element with that key from the map
 */
int hashmap_remove_int(hashmap_map *m, uint16_t key) {
	int i;
	int curr;

	/* Find key */
	curr = hashmap_hash(m, key);

	/* Linear probing, if necessary */
	for(i = 0; i<MAX_CHAIN_LENGTH; i++) {

        uint8_t in_use = m->data[curr].in_use;
        if (in_use == 1) {
            if (m->data[curr].key == key) {
                /* Blank out the fields */
                m->data[curr].in_use = 0;
                //m->data[curr].data = NULL;
                //m->data[curr].key = NULL;

                /* Reduce the size */
                m->size--;
                return MAP_OK;
            }
		}
		curr = (curr + 1) % m->table_size;
	}

	printf("Error : element with key addr %u.%u could not be found and thus wasn't removed\n", key >> 8, key & 0x0F);
	/* Data not found */
	return MAP_MISSING;
}

/**
 * Removes an element with that key from the map
 */
int hashmap_remove(hashmap_map *m, linkaddr_t key) {
    return hashmap_remove_int(m, linkaddr2uint16_t(key));
}

/**
 * Deallocates the hashmap
 */
void hashmap_free(hashmap_map *m) {
	free(m->data);
	free(m);
}

/**
 * Returns the length of the hashmap (0 in the null case)
 */
int hashmap_length(hashmap_map *m) {
	if(m != NULL) return m->size;
	else return 0;
}

/**
 * Prints the content of the hashmap
 */
void hashmap_print(hashmap_map *m) {
	hashmap_element* map = m->data;
	int i;
	for (i = 0; i < m->table_size; i++) {
		hashmap_element elem = *(map+i);
		if (elem.in_use) {
			printf("%u.%u; reachable from %u.%u\n",
				elem.key >> 8, elem.key & 0x0f, elem.data.u8[0], elem.data.u8[1]);
		}
	}
}

/**
 * Removes entries that have timeout (based on arguments current_time and timeout_delay)
 * Design choice : unsigned long overflow is not taken into account since it would wrap up in ~= 135 years
 */
void hashmap_delete_timeout(hashmap_map *m, unsigned long current_time, unsigned long timeout_delay) {
	hashmap_element *runner = m->data;
	int i;
	for (i = 0; i < m->table_size; i++) {
		if (runner[i].in_use && current_time > runner[i].time+timeout_delay) {
			// entry timeout
			runner[i].in_use = 0;
			printf("Node with addr %u.%u timed out -> deleted\n", runner[i].key >> 8, runner[i].key & 0x0f);
		}
	}
}

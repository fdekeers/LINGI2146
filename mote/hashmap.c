#include "hashmap.h"

/**
 * Converts a linkaddr_t to a uint16_t
 */
uint16_t linkaddr2uint16_t (linkaddr_t x) {
	return x.u16;
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
	if(2*m->size >= m->table_size) {
		if (DEBUG_MODE) printf("Map at least half full, resizing\n");
		return MAP_FULL;
	}

	/* Find the best index */
	curr = key % m->table_size;
	if (DEBUG_MODE) printf("Best index for key %u is %d\n", key, curr);

	/* Value of index if found */
	int firstInd = MAP_FULL;

	/* Linear probing */
	for(i = 0; i< MAX_CHAIN_LENGTH; i++) {
		if(m->data[curr].in_use == 0) {
			if (firstInd == MAP_FULL) {
				firstInd = curr;
			}
		} else if(m->data[curr].in_use == 1 && (m->data[curr].key == key)) {
			if (firstInd != MAP_FULL) {
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
 * Fills the new_array of data based on the old_array and modifies the different
 * elements of the hashmap m accordingly.
 * Return value : MAP_OK if everything was fine, MAP_OMEM if there is a bug that
 * 		  should not even be there, or MAP_FULL if we have to rehash
 */
int hashmap_fill_rehash(hashmap_map *m, hashmap_element *old_array, int old_table_size, hashmap_element *new_array, int new_table_size) {
	/* Update the array */
	m->data = new_array;

	/* Update the sizes */
	m->table_size = new_table_size;
	m->size = 0;


	/* Rehash the elements */
	int i;
	for(i = 0; i < old_table_size; i++) {
		if (DEBUG_MODE) printf("Rehashing, i : %d, old_table_size : %d\n", i, old_table_size);
		if (old_array[i].in_use == 0)
		    continue;
	    	uint8_t isRehashing = 1;
		int status = hashmap_put_int(m, old_array[i].key, old_array[i].data, old_array[i].time, isRehashing);
		if (status == MAP_FULL) {
			// have to rehash for more memory !
			return status;
		}
		if (status == MAP_OMEM) {
			printf("ERROR : MAP_OMEM in simple rehash, should not happen\n");
			return MAP_OMEM;
		}
	}

	return MAP_OK;
}

/**
 * Changes the size of the hashmap (at least the double + 1, more if multiple
 * rehashes have to be made) and rehashes all the elements.
 */
int hashmap_rehash(hashmap_map *m) {
	if (DEBUG_MODE) {
		printf("Rehashing hashmap, printing before\n");
		hashmap_print(m);
	}

	/* Constants needed for all rehash operations */
	hashmap_element* curr = m->data; // pointer to old data
	int old_table_size = m->table_size;
	int previous_table_size = old_table_size; // size of previous rehash

	int status = MAP_FULL;
	do {
		int new_table_size = previous_table_size*2 + 1;
		/* Setup the new elements */
		hashmap_element* temp = (hashmap_element *)
		my_calloc(new_table_size, sizeof(hashmap_element));
		if(!temp) {
			printf("MAP_OMEM when tried to alloc %d elements of size %d\n", 2 * m->table_size, sizeof(hashmap_element));
			return MAP_OMEM;
		}

		status = hashmap_fill_rehash(m, curr, old_table_size, temp, new_table_size);
		if (status == MAP_FULL) {
			free(temp); // we'll have to rehash
		}
		previous_table_size = new_table_size;
	} while (status == MAP_FULL); // have to rehash again
	
	if (status == MAP_OMEM) {
		// should not happen
		printf("Hashmap rehash encountered MAP_OMEM, not normal\n");
		return MAP_OMEM;
	}


	free(curr); // free old data
	if (DEBUG_MODE) {
		printf("Hashmap rehashed, printing new\n");
		hashmap_print(m);
	}

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
 * Return value : MAP_OMEM if out of memory, MAP_FULL if rehash has to be called
 *		  while already called by rehash, MAP_NEW if an element was added,
 * 		  MAP_UPDATE if an element was updated.
 */
int hashmap_put_int(hashmap_map *m, uint16_t key, linkaddr_t value, unsigned long time, uint8_t isRehashing) {
	if (DEBUG_MODE) printf("Trying to put node %u. Rehashing : %d\n", key, isRehashing);
	if (!isRehashing && DEBUG_MODE) {
		hashmap_print(m);
	}
	int index;
	int ret = MAP_UPDATE;

	/* Find a place to put our value */
	index = hashmap_hash(m, key);
	while(index == MAP_FULL) {
		if (isRehashing) {
			// to be sure we don't call rehash while already in rehash
			if (DEBUG_MODE) printf("MAP_FULL when already rehashing -> double rehash at least\n");
			return MAP_FULL;
		}
		if (hashmap_rehash(m) == MAP_OMEM) {
			printf("Out of memory when trying to put node %u\n", key);
			return MAP_OMEM;
		}
		index = hashmap_hash(m, key);
	}

	/* Set the data */
	if (!m->data[index].in_use) {
		ret = MAP_NEW;
		m->size++; // we are adding, not updating
		m->data[index].in_use = 1;
	}
	m->data[index].data = value;
	m->data[index].time = time;
	m->data[index].key = key;
	if (DEBUG_MODE) printf("Node with key %u added\n",key);

	return ret;

}

/**
 * Adds a pointer to the hashmap with some key
 * If the element was already present, the data is overwritten with the new one
 * Return value : MAP_OMEM if out of memory, MAP_NEW if an element was added,
 * 		  MAP_UPDATE if an element was updated.
 */
int hashmap_put(hashmap_map *m, linkaddr_t key, linkaddr_t value) {
	unsigned long time = clock_seconds();
	uint8_t isRehashing = 0;
	return hashmap_put_int(m, linkaddr2uint16_t(key), value, time, isRehashing);
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

                /* Reduce the size */
                m->size--;
		if (DEBUG_MODE) printf("Node with key %u was removed from hashmap\n",key);
                return MAP_OK;
            }
		}
		curr = (curr + 1) % m->table_size;
	}

	if (DEBUG_MODE) printf("Error : element with key addr %u could not be found and thus wasn't removed\n", key);
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
	printf("Printing hashmap\n");
	hashmap_element* map = m->data;
	int i;
	for (i = 0; i < m->table_size; i++) {
		hashmap_element elem = *(map+i);
		if (elem.in_use) {
			printf("index %d : %u; reachable from %u\n",
				i, elem.key, linkaddr2uint16_t(elem.data));
		}
	}
}

/**
 * Removes entries that have timed out (based on current time and TIMEOUT_CHILDREN)
 * Design choice : unsigned long overflow is not taken into account since it would wrap up in ~= 135 years
 * Returns 1 if at least one element has been removed, 0 if no element has been removed.
 */
int hashmap_delete_timeout(hashmap_map *m) {
	int ret = 0;
	hashmap_element *runner = m->data;
	int i;
	for (i = 0; i < m->table_size; i++) {
		if (runner[i].in_use && clock_seconds() > runner[i].time + TIMEOUT_CHILDREN) {
			// entry timeout
			runner[i].in_use = 0;
			printf("Node with addr %u timed out -> deleted\n", runner[i].key);
			ret = 1;
		}
	}
	return ret;
}

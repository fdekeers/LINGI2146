#include "hashmap.h"

/*
 * Convert linkaddr to a uint16_t
 */
uint16_t linkaddr2uint16_t (linkaddr_t x) {
    uint8_t a = x.u8[0]; uint8_t b = x.u8[1];
    uint16_t newval = (((uint16_t) a) << 8 ) | ((uint16_t) b);
    return newval;
}

void * my_calloc(int nmemb, int size) {
	void * malloced = malloc(nmemb*size);
	if (!malloced) {
		return NULL;
	}
	memset(malloced, 0, nmemb*size);
	return malloced;
}

/*
 * Return an empty hashmap, or NULL on failure.
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

/*
 * Return the integer of the location in data
 * to store the point to the item, or MAP_FULL.
 */
int hashmap_hash(hashmap_map *m, uint16_t key){
	int curr;
	int i;

	/* If full, return immediately */
	if(m->size >= (m->table_size/2)) return MAP_FULL;

	/* Find the best index */
	curr = key % m->table_size;

	/* Linear probing */
	for(i = 0; i< MAX_CHAIN_LENGTH; i++){
		if(m->data[curr].in_use == 0)
			return curr++;  //mutated statement

		if(m->data[curr].in_use == 1 && (m->data[curr].key == key))
			return curr;
		
		curr = (curr + 1) % m->table_size;
	}

	return MAP_FULL;
}



/*
 * Doubles the size of the hashmap, and rehashes all the elements
 */
int hashmap_rehash(hashmap_map *m){
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

/*
 * Add a pointer to the hashmap with some key
 */
int hashmap_put_int(hashmap_map *m, uint16_t key, linkaddr_t value){
	unsigned long time = clock_seconds();
	int index;

	/* Find a place to put our value */
	index = hashmap_hash(m, key);
	while(index == MAP_FULL){
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

/*
 * Add a pointer to the hashmap with some key.
 */
int hashmap_put(hashmap_map *m, linkaddr_t key, linkaddr_t value){
    return hashmap_put_int(m, linkaddr2uint16_t(key), value);
}
/*
 * Get your pointer out of the hashmap with a key
 */
int hashmap_get_int(hashmap_map *m, uint16_t key, linkaddr_t *arg){
	int curr;
	int i;

	/* Find data location */
	curr = hashmap_hash(m, key);

	/* Linear probing, if necessary */
	for(i = 0; i<MAX_CHAIN_LENGTH; i++){

        uint8_t in_use = m->data[curr].in_use;
        if (in_use == 1){
            if (m->data[curr].key == key){
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

/*
 * Get your pointer out of the hashmap with a key
 */
int hashmap_get(hashmap_map *m, linkaddr_t key, linkaddr_t *arg){
    return hashmap_get_int(m, linkaddr2uint16_t(key), arg);
}

/*
 * Remove an element with that key from the map
 */
int hashmap_remove_int(hashmap_map *m, uint16_t key){
	int i;
	int curr;

	/* Find key */
	curr = hashmap_hash(m, key);

	/* Linear probing, if necessary */
	for(i = 0; i<MAX_CHAIN_LENGTH; i++){

        uint8_t in_use = m->data[curr].in_use;
        if (in_use == 1){
            if (m->data[curr].key == key){
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

	/* Data not found */
	return MAP_MISSING;
}

/*
 * Remove an element with that key from the map
 */
int hashmap_remove(hashmap_map *m, linkaddr_t key){
    return hashmap_remove_int(m, linkaddr2uint16_t(key));
}

/* Deallocate the hashmap */
void hashmap_free(hashmap_map *m){
	free(m->data);
	free(m);
}

/* Return the length of the hashmap */
int hashmap_length(hashmap_map *m){
	if(m != NULL) return m->size;
	else return 0;
}

/* Bijan Oviedo(boviedo)
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include "pipe.h"
#include <inttypes.h>

cache_t *cache_new(uint32_t sets_per_cache, uint32_t lines_per_set, uint32_t bytes_per_block) {
	cache_t* output_cache = (cache_t*)malloc(sizeof(cache_t));
	output_cache->sets_per_cache = sets_per_cache;
	output_cache->lines_per_set = lines_per_set;
	output_cache->words_per_block = bytes_per_block / 4;
	output_cache->lines = (cache_line**)malloc(sets_per_cache * sizeof(cache_line*));
	output_cache->results.hits = 0;
	output_cache->results.misses = 0;
	output_cache->results.evicts = 0;
	int i;
	for (i = 0; i < sets_per_cache; i++) {
		output_cache->lines[i] = (cache_line*)malloc(lines_per_set * sizeof(cache_line));
		int j;
		for (j = 0; j < lines_per_set; j++) {
			output_cache->lines[i][j].valid_bit = 0;
			output_cache->lines[i][j].dirty_bit = 0;
			output_cache->lines[i][j].tag = 0;
			output_cache->lines[i][j].LRUcounter = 0;
			output_cache->lines[i][j].blocks = (uint32_t*)malloc((bytes_per_block / 4) * sizeof(uint32_t));
			int k;
			for (k = 0; k < bytes_per_block / 4; k++) {
				output_cache->lines[i][j].blocks[k] = 0;
			}
		}
	}
	return output_cache;
}

void cache_destroy(cache_t *c)
{
	uint32_t sets_per_cache = c->sets_per_cache, lines_per_set = c->lines_per_set;
	int i;
	for (i = 0; i < sets_per_cache; i++) {
		int j;
		for (j = 0; j < lines_per_set; j++) {
			free(c->lines[i][j].blocks);
		}
		free(c->lines[i]);
	}
	free(c->lines);
	free(c);
}


uint32_t index_of_max_value(int* array, uint32_t n) {
	// returns the index of the maximum value in the array
	uint32_t max_index = 0, max_value = 0;
	uint32_t i;
	for (i = 0; i < n; i++) {
		if (array[i] > max_value) {
			max_value = array[i];
			max_index = i;
		}
	}
	return max_index;
}

uint32_t* blockbuilder(uint64_t addr) { //given og pc address, calculate the 32 bit instructions of the 7 next instructions.
	uint32_t i = 0;
	uint64_t tmpaddr = addr;
	uint32_t* myArray = (uint32_t*)malloc(sizeof(uint32_t) * 8);
	for (i = 0; i < 8; i++, tmpaddr += 4) {
		myArray[i] = mem_read_32(tmpaddr);
	}
	return myArray;
}

bool i_cache_did_hit(cache_t *c, uint64_t addr) {
	uint32_t set_index = (addr >> 5) & 0x3F; //set index
	uint64_t tag = (addr >> 11) & 0x1FFFFFFFFFFFFF; //addre for ic is CURRENT STATE PC 
	uint32_t num_blocks = c->lines_per_set;
	uint32_t line_index;
	for (line_index = 0; line_index < num_blocks; line_index++) {
		if (c->lines[set_index][line_index].valid_bit == 0) {// cache empty, update in first available empty slot
			return FALSE;
		} else if (c->lines[set_index][line_index].tag == tag) {// if cache hit
			return TRUE;
		} 
	}
	return FALSE;
}

uint32_t i_cache_update(cache_t *c, uint64_t addr) {
	uint32_t set_index = (addr >> 5) & 0x3F; //set index
	uint64_t tag = (addr >> 11) & 0x1FFFFFFFFFFFFF; //addre for ic is CURRENT STATE PC 
	uint32_t byteoffset = (addr & 0x1F) / 4;
	uint32_t* blockArray = blockbuilder(addr);
	uint32_t num_blocks = c->lines_per_set;
	uint32_t line_index;
	for (line_index = 0; line_index < num_blocks; line_index++) {
		c->lines[set_index][line_index].LRUcounter++;
	} // increment lru counter for ALL entries in this set. only one of them will be used at most, and when it hits, we set lru=0 for that one.
	for (line_index = 0; line_index < num_blocks; line_index++) { // traverses through the different associativity

		if (c->lines[set_index][line_index].valid_bit == 0) {// cache empty, update in first available empty slot
			c->lines[set_index][line_index].LRUcounter = 0;
			c->lines[set_index][line_index].valid_bit = 1;
			c->lines[set_index][line_index].tag = tag;
			c->lines[set_index][line_index].blocks = blockArray;
			c->results.misses++;
			c->results.most_recent_type = MISS;

			return c->lines[set_index][line_index].blocks[byteoffset]; //end here since we updated the cache
		} else if (c->lines[set_index][line_index].tag == tag) {// if cache hit

			c->lines[set_index][line_index].LRUcounter = 0;
			c->results.hits++;
			c->results.most_recent_type = HIT;

			return c->lines[set_index][line_index].blocks[byteoffset]; // cache hit! return requested block
		} 
	} 
	// cache is full, need to evict using lru
	uint32_t lru_array[num_blocks];
	int i;
	for (i = 0; i < num_blocks; i++) {
		lru_array[i] = c->lines[set_index][i].LRUcounter;
	}

	uint32_t evicted_index = index_of_max_value(lru_array, num_blocks);
	c->lines[set_index][evicted_index].valid_bit = 1;
	c->lines[set_index][evicted_index].tag = tag;
	// Have to retrieve from memory the data
	c->lines[set_index][evicted_index].blocks = blockArray; 
	c->lines[set_index][evicted_index].LRUcounter = 0;
	c->results.misses++;
	c->results.most_recent_type = MISS;
	return c->lines[set_index][evicted_index].blocks[byteoffset];
}


int64_t d_cache_update(cache_t *c, instruction inst, uint64_t addr, int64_t new_store_data, int64_t* my_output) {
	// if the instruction requires 64 bits, returns 32 bits in out parameter
	// call cache_update(DEST_ADDR, 1); in Mem stage of pipeline.
	uint32_t set_index = (addr >> 5) & 0xFF;
	uint64_t tag = (addr >> 13) & 0x7FFFFFFFFFFFF;
	uint32_t byte_offset = addr & 0x1F;

	if (inst >= LDUR && inst <= LDURH) { //read memory request
		int i = 0;
		for (i = 0; i < 8; i++) { //lru counter everything that's valid/in the cache
			c->lines[set_index][i].LRUcounter++;
		}
		for (i = 0; i < 8; i++) { // did cache hit?
			if (c->lines[set_index][i].valid_bit == 1 && c->lines[set_index][i].tag == tag) {
				c->results.hits++;
				c->results.most_recent_type = HIT;

				printf("dcache hit (0x%" PRIx64 ") at cycle %d\n", addr, stat_cycles);
			//	exit(1);
				int64_t second_word = c->lines[set_index][i].blocks[byte_offset/4] & 0x00000000FFFFFFFF;
				if (inst == LDUR) {
					int64_t first_word = (((int64_t) c->lines[set_index][i].blocks[byte_offset/4 + 1]) << 32) & 0xFFFFFFFF00000000;
					int64_t combined = first_word | second_word;
					*my_output = combined;
					return combined;
				} else {
					return second_word;
				}
			}
		}
		printf("dcache miss (0x%" PRIx64 ") at cycle %d\n", addr, stat_cycles);
		for (i = 0; i < 8; i++) {
			if (c->lines[set_index][i].valid_bit == 0) { // if cache empty, no need to check for dirty bit
				uint32_t* blockArray = blockbuilder(addr); // read from lower memory of dest address
				c->lines[set_index][i].valid_bit = 1;
				c->lines[set_index][i].dirty_bit = 0;
				c->lines[set_index][i].tag = tag;
				c->lines[set_index][i].blocks = blockArray;

				free(blockArray);
				c->lines[set_index][i].LRUcounter = 0;
				c->results.misses++;
				c->results.most_recent_type = MISS;
				return c->lines[set_index][i].blocks[byte_offset / 4]; //or should it be blocks[0] the first element?
			}
		}

		uint32_t lru_array[8];

		for (i = 0; i < 8; i++) {
			lru_array[i] = c->lines[set_index][i].LRUcounter;
		}
		 //if cache full and tags dont match. need to evict
		uint32_t evicted_index = index_of_max_value(lru_array, 8); //we evict this old cache block
		if (c->lines[set_index][evicted_index].dirty_bit == 1) {
			int block_index;
			for (block_index = 0; block_index < 8; block_index++) { // writes the cache data in to lower memory
				mem_write_32(addr, c->lines[set_index][evicted_index].blocks[block_index]);
			}
			uint32_t* blockArray= blockbuilder(addr); // read back from lower memory of dest address
			c->lines[set_index][evicted_index].valid_bit = 1;
			c->lines[set_index][evicted_index].dirty_bit = 0;
			c->lines[set_index][evicted_index].tag = tag;
			c->lines[set_index][evicted_index].blocks = blockArray;
			c->lines[set_index][evicted_index].LRUcounter = 0;
			c->results.misses++;
			c->results.most_recent_type = MISS;
			return c->lines[set_index][evicted_index].blocks[byte_offset/4];
		}

	} else { // STORE, write request
		int i = 0;

		for (i = 0; i < 8; i++) { //lru counter everything that's valid/in the cache
			if (c->lines[set_index][i].valid_bit == 1) {
				c->lines[set_index][i].LRUcounter++;
			}
		}
		for (i = 0; i < 8; i++) { // did cache hit?
			if (c->lines[set_index][i].valid_bit == 1 && c->lines[set_index][i].tag == tag) {
				c->results.hits++;
				c->results.most_recent_type = HIT;
				// writing new data into cache block
				if (inst == STUR) {
					c->lines[set_index][i].blocks[byte_offset/4 + 1] = (uint32_t)((new_store_data >> 32) & 0xFFFFFFFF);
					c->lines[set_index][i].blocks[byte_offset/4] = (uint32_t)(new_store_data & 0xFFFFFFFF);
				} else {
					c->lines[set_index][i].blocks[byte_offset / 4] = (uint32_t)new_store_data;
				}
				printf("dcache hit (%" PRIx64 ") at cycle %d\n", addr, stat_cycles);
				c->lines[set_index][i].dirty_bit = 1;
				return 0;
			}
		}
		//cache miss
		//cache empty, so not dirty

		printf("dcache miss (%" PRIx64 ") at cycle %d\n", addr, stat_cycles);
		for (i = 0; i < 8; i++) {
			if (c->lines[set_index][i].valid_bit == 0) { // if cache empty, no need to check for dirty bit
				uint32_t* blockArray = blockbuilder(addr); // read from lower memory of dest address
				int k;
				for (k = 0; k < 8; k++) {
					if (blockArray[k] != 0) {
						printf("%d [%d] %d\n",inst,k,blockArray[k]);
					}
				}
				
				c->lines[set_index][i].valid_bit = 1;
				c->lines[set_index][i].dirty_bit = 1; // need to set dirty bit 1 anyways later cuz we writing into cache block
				c->lines[set_index][i].tag = tag;
				c->lines[set_index][i].blocks = blockArray;
				c->lines[set_index][i].LRUcounter = 0;
				c->results.misses++;
				c->results.most_recent_type = MISS;
				if (inst == STUR) {
					c->lines[set_index][i].blocks[byte_offset/4 + 1] = (uint32_t)((new_store_data >> 32) & 0xFFFFFFFF);
					c->lines[set_index][i].blocks[byte_offset/4] = (uint32_t)(new_store_data & 0xFFFFFFFF);
				} else {
					c->lines[set_index][i].blocks[byte_offset / 4] = (uint32_t)new_store_data;
				}
				return 0; 
			}
		}

		//cache full, but miss, need to evict, and check for dirty bit

		uint32_t lru_array[8];
		for (i = 0; i < 8; i++) {
			lru_array[i] = c->lines[set_index][i].LRUcounter;
		}
		 //if cache full and tags dont match. need to evict
		uint32_t evicted_index = index_of_max_value(lru_array, 8); //we evict this old cache block

		int block_index;
		if (c->lines[set_index][evicted_index].dirty_bit == 1) {
			for(block_index = 0; block_index < 8; block_index++) { // writes the cache data in to lower memory
				printf("writing back to memory in store\n\n");
				//exit(0);
				mem_write_32(addr, c->lines[set_index][evicted_index].blocks[block_index]);
				if (inst == STUR) {
					mem_write_32(addr + 4, c->lines[set_index][evicted_index].blocks[block_index + 1]);
				}
			}
		}
		int* blockArray = blockbuilder(addr); // read back from lower memory of dest address
		c->lines[set_index][evicted_index].valid_bit = 1;
		c->lines[set_index][evicted_index].dirty_bit = 1;
		c->lines[set_index][evicted_index].tag = tag;
		c->lines[set_index][evicted_index].blocks = blockArray;
		c->lines[set_index][evicted_index].LRUcounter = 0;
		c->results.misses++;
		c->results.most_recent_type = MISS;
		if (inst == STUR) {
			c->lines[set_index][i].blocks[byte_offset / 4 + 1] = (uint32_t)((new_store_data >> 32) & 0xFFFFFFFF);
			c->lines[set_index][i].blocks[byte_offset / 4] = (uint32_t)(new_store_data & 0xFFFFFFFF);
		} else {
			c->lines[set_index][i].blocks[byte_offset / 4] = (uint32_t)new_store_data;
		}
		return 0; 
	}
}
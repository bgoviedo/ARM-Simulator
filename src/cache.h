/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

typedef struct cache_line {
  uint32_t valid_bit;
  uint32_t dirty_bit;
  uint64_t tag;
  uint32_t* blocks; //32byte block. 4 byte per instr. put the next 7 instrs after original for spatial locality in cache. 64bit pc -> 32bit instr in decode. do that 7 times more in decode and store in this block array.
  uint32_t LRUcounter;
} cache_line;

typedef enum cache_result_type {
  MISS,
  HIT
} cache_result_type;


typedef struct cache_results {
  uint32_t hits;
  uint32_t misses;
  uint32_t evicts;
  cache_result_type most_recent_type;
} cache_results;


typedef struct cache_t {
  cache_line** lines;
  uint32_t sets_per_cache;
  uint32_t lines_per_set; // = ways
  uint32_t words_per_block;
  cache_results results;

} cache_t;

cache_t* i_cache;
cache_t* d_cache;

// cache line is [t][v][d][bloooooock]

cache_t *cache_new(uint32_t sets_per_cache, uint32_t lines_per_set, uint32_t bytes_per_block);
void cache_destroy(cache_t *c);

uint32_t cache_update(cache_t *c, uint64_t addr, uint32_t is_instruction_cache);


#endif
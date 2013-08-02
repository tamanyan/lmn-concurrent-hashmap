/**
 * License GPL3
 * @file   lmn_hopscotch_hashmap.c
 * @brief  hopscotch hash table implementation
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "lmn_hopscotch_hashmap.h"

#define LMN_HOPCH_INIT_SIZE 2 << 16

/**
 * private functions
 */

inline unsigned int get_nearest_power_of_two(const unsigned int value) {
  unsigned int rc = 1;
  while(rc < value) {
    rc <<= 1;
  }
  return rc;
}

inline unsigned int calc_divide_shift(const unsigned int value) {
  unsigned int num_shift = 0;
  unsigned int curr = 1;
  while(curr < value) {
    curr <<= 1;
    ++num_shift;
  }
  return num_shift;
}

/**
 * public functions
 */
void lmn_hopch_hashmap_init(lmn_hopch_hashmap_t *map) {
  unsigned int concurrencyLevel  = 12;
  unsigned int num_buckets = LMN_HOPCH_INIT_SIZE + LMN_HOPCH_INSERT_RANGE + 1;
  map->bucket_mask      = LMN_HOPCH_INIT_SIZE - 1;
  map->segment_mask     = get_nearest_power_of_two(concurrencyLevel-1);
  map->segment_shift    = calc_divide_shift(get_nearest_power_of_two(concurrencyLevel/get_nearest_power_of_two(concurrencyLevel)) - 1);
  map->buckets          = lmn_malloc(num_buckets, lmn_hopch_bucket_t);
  map->segments         = lmn_malloc(map->segment_mask, lmn_hopch_segment_t);
  memset(map->buckets, 0x00, sizeof(lmn_hopch_bucket_t)*num_buckets);
  memset(map->segments, 0x00, sizeof(lmn_hopch_segment_t)*map->segment_mask);
  dbgprint("segment_mask : %d, segment_shift : %d bucket_mask : %d\n", map->segment_mask, map->segment_shift, map->bucket_mask);
}

void lmn_hopch_hashmap_free(lmn_hopch_hashmap_t *map) {
}

lmn_data_t lmn_hopch_hashmap_put(const lmn_hopch_hashmap_t *map, const lmn_key_t key, const lmn_data_t data) {
  lmn_hash_t hash               = lmn_hash_calc(key);
  lmn_hopch_segment_t* segment  = map->segments + hash;
  return NULL;
}

lmn_data_t lmn_hopch_hashmap_find(const lmn_hopch_hashmap_t *map, const lmn_key_t key) {
  return NULL;
}

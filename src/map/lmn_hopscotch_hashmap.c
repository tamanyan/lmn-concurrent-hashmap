/**
 * License GPL3
 * @file   lmn_hopscotch_hashmap.c
 * @brief  hopscotch hash table implementation
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "lmn_hopscotch_hashmap.h"

#define LMN_HOPCH_INIT_SIZE (32 * 1024)

/**
 * private functions
 */

inline lmn_word get_nearest_power_of_two(const lmn_word value) {
  lmn_word rc = 1;
  while (rc < value) {
    rc <<= 1;
  }
  return rc;
}

inline lmn_word calc_divide_shift(const lmn_word value) {
  lmn_word num_shift = 0;
  lmn_word curr = 1;
  while (curr < value) {
    curr <<= 1;
    ++num_shift;
  }
  return num_shift;
}

inline int get_first_msb_bit_index(const lmn_word x) {
  if (0==x)
    return -1;
#ifdef __x86_64__
  return  __builtin_clzll(x)-1;
#else
  return  __builtin_clz(x)-1;
#endif
}

inline int get_first_lsb_bit_index(const lmn_word x) {
  if (0==x)
    return -1;
#ifdef __x86_64__
  return  __builtin_ffsll(x)-1;
#else
  return  __builtin_ffs(x)-1;
#endif
}

inline int dump_map(lmn_hopch_hashmap_t *map) {
  dbgprint("dump map\n", NULL);
  dbgprint("segment_mask : 0x%lu\nsegment_shift : 0x%lu\nbucket_mask : 0x%lu\n", 
            map->segment_mask, map->segment_shift, map->bucket_mask);
  dbgprint("buckets : %p\nsegments : %p\n", map->buckets, map->segments);
}

/**
 * public functions
 */
void lmn_hopch_hashmap_init(lmn_hopch_hashmap_t *map) {
  lmn_word concurrencyLevel  = 1;
  lmn_word num_buckets = LMN_HOPCH_INIT_SIZE + LMN_HOPCH_INSERT_RANGE + 1;

  map->bucket_mask      = LMN_HOPCH_INIT_SIZE - 1;
  map->segment_mask     = get_nearest_power_of_two(concurrencyLevel-1);
  map->segment_shift    = calc_divide_shift(
      get_nearest_power_of_two(concurrencyLevel/get_nearest_power_of_two(concurrencyLevel)) - 1);

  // dbgprint("0x%x\n", get_first_msb_bit_index(map->segment_mask));
  // map->segment_shift    = get_first_msb_bit_index(map->bucket_mask) - get_first_msb_bit_index(map->segment_mask);
  map->buckets          = lmn_malloc(num_buckets, lmn_hopch_bucket_t);
  map->segments         = lmn_malloc(map->segment_mask + 1, lmn_hopch_segment_t);

  memset(map->buckets, 0x00, sizeof(lmn_hopch_bucket_t)*num_buckets);
  memset(map->segments, 0x00, sizeof(lmn_hopch_segment_t)*(map->segment_mask + 1));
  dump_map(map);
}

void lmn_hopch_hashmap_free(lmn_hopch_hashmap_t *map) {
}

lmn_data_t lmn_hopch_hashmap_put(lmn_hopch_hashmap_t *map, const lmn_key_t key, const lmn_data_t data) {
  lmn_hash_t hash               = lmn_hash_calc(key);
  lmn_hopch_segment_t *segment  = &map->segments[(hash >> map->segment_shift) & map->segment_mask];
  /* TODO
   * require segment lock
   */
  lmn_hopch_bucket_t *start_bucket = &map->buckets[hash & map->bucket_mask]; 

  // check if aready contain
  {
    lmn_word hop_info = start_bucket->hop_info;
    int i;
    lmn_hopch_bucket_t *cur;
    while (hop_info != 0) {
      i   = get_first_lsb_bit_index(hop_info);
      cur = start_bucket + i;
      if (hash == cur->hash && key == cur->key) {
        /* TODO
         * require segment unlock
         */
        return cur->data;
      }
      // next setting bit
      hop_info &= ~(1U << i);
    }
  }
  
  // lock for free bucket
  {
    lmn_hopch_bucket_t *free_bucket = start_bucket;
    int free_distance = 0;
    for (; free_distance < LMN_HOPCH_INSERT_RANGE; ++free_distance, ++free_bucket) {
      if (free_bucket->hash == LMN_HASH_EMPTY && LMN_CAS(&(free_bucket->hash), LMN_HASH_EMPTY, LMN_HASH_BUSY))
        break;
    }
    dbgprint("0x%lu\n", free_bucket->hash);
  }
  return NULL;
}

lmn_data_t lmn_hopch_hashmap_find(const lmn_hopch_hashmap_t *map, const lmn_key_t key) {
  return NULL;
}

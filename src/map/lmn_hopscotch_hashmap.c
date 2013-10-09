/**
 * @file   lmn_hopscotch_hashmap.c
 * @brief  hopscotch hash table implementation
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "lmn_hopscotch_hashmap.h"

#define LMN_HOPCH_INIT_SIZE (32 * 1024)

#ifdef __linux__

inline lmn_hash_t lmn_hash_calc(lmn_key_t key) {
#ifdef __x86_64__
  /* Robert Jenkins' 32 bit Mix Function */
  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);

  /* Knuth's Multiplicative Method */
  key = (key >> 3) * 2654435761;
#else
  key ^= (key << 15) ^ 0xcd7dcd7d;
  key ^= (key >> 10);
  key ^= (key <<  3);
  key ^= (key >>  6);
  key ^= (key <<  2) + (key << 14);
  key ^= (key >> 16);
#endif
  return key;
}

inline int lmn_hash_eq(lmn_hash_t h1, lmn_hash_t h2) {
  return h1 == h2;
}

inline int lmn_hash_key_eq(lmn_hash_t key1, lmn_hash_t key2) {
  return key1 == key2;
}

#endif
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

void find_closer_free_bucket(lmn_hopch_hashmap_t *map, const lmn_hopch_segment_t *start_segment, lmn_hopch_bucket_t **free_bucket, int *free_distance) {
  lmn_hopch_bucket_t *move_bucket = *free_bucket - (LMN_HOPCH_HOP_RANGE - 1);
  lmn_hopch_segment_t *move_segment;
  u32 move_free_dist, move_new_free_distance, mask, i;
  lmn_word start_hop_info;
  for (move_free_dist = LMN_HOPCH_HOP_RANGE - 1; move_free_dist > 0; --move_free_dist) {
    start_hop_info         = move_bucket->hop_info;
    move_new_free_distance = -1;
    mask                   = 1;
    for (i = 0; i < move_free_dist; ++i, mask <<= 1) {
      if (mask & start_hop_info) {
        move_new_free_distance = i;
        break;
      }
    }
    if (move_new_free_distance != -1) {
      move_segment = &map->segments[((move_bucket - map->buckets) >> map->segment_shift ) & map->segment_mask];
      
      if (start_segment != move_segment) {
        /* TODO:
         * require segment lock
         */
      }

      if (start_hop_info == move_bucket->hop_info) {
        lmn_hopch_bucket_t *new_free_bucket = move_bucket + move_new_free_distance;
        (*free_bucket)->data = new_free_bucket->data;
        (*free_bucket)->key  = new_free_bucket->key;
        (*free_bucket)->hash = new_free_bucket->hash;

        ++(move_segment->timestamp);

        move_bucket->hop_info |= (1U << move_free_dist);
        move_bucket->hop_info &= ~(1U << move_new_free_distance);

        *free_bucket    = new_free_bucket;
        *free_distance -= move_free_dist;
        if(start_segment != move_segment) {
          /* TODO:
           * require segment unlock
           */
        }
        return;
      }
      if(start_segment != move_segment) {
        /* TODO:
         * require segment unlock
         */
      }
    }
    ++move_bucket;
  }
  *free_bucket  = NULL;
  *free_distance = 0;
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
  int i;
  /* TODO:
   * require segment lock
   */
  lmn_hopch_bucket_t *start_bucket = &map->buckets[hash & map->bucket_mask]; 
  lmn_word hop_info = start_bucket->hop_info;

  if (key == 0x2000) {
    dbgprint("exist key : %lu, hash : %lu, data : %p\n",key, hash, data);
  }
  // check if aready contain
  {
    lmn_hopch_bucket_t *cur;
    while (hop_info != 0) {
      i   = get_first_lsb_bit_index(hop_info);
      cur = start_bucket + i;
      if (hash == cur->hash && key == cur->key) {
        /* TODO:
         * require segment unlock
         */
        dbgprint("exist key : %lu, hash : %lu, data : %p\n",key, hash, data);
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

    if (free_distance < LMN_HOPCH_INSERT_RANGE) {
      do {
        if (free_distance < LMN_HOPCH_HOP_RANGE) {
          free_bucket->data     = data;
          free_bucket->key      = key;
          free_bucket->hash     = hash;
          free_bucket->hop_info |= (1U << free_distance);
          dbgprint("insert key : %lu, hash : %lu, data : %p\n",key, hash, data);
          /* TODO:
           * require segment unlock
           */
          return data;
        }
      } while (free_bucket != 0);
    }
  }

  fprintf(stderr, "ERROR - RESIZE is not implemented");
  exit(1);
  return LMN_HASH_EMPTY_DATA;
}

lmn_data_t lmn_hopch_hashmap_find(const lmn_hopch_hashmap_t *map, const lmn_key_t key) {
  return NULL;
}

/**
 * License GPL3
 * @file   lmn_hopscotch_hashmap.h
 * @brief  hopscotch hash table implementation
 * @author Taketo Yoshida
 */
#ifndef LMN_HOPSCOTCH_HASHMAP_H
#  define LMN_HOPSCOTCH_HASHMAP_H

#include "lmn_map_util.h"
#include "lmn_hash.h"

typedef struct {
  lmn_word      volatile hop_info;
  lmn_word      volatile hash;
  lmn_key_t     volatile key;
  lmn_data_t    volatile data;
} lmn_hopch_bucket_t;

typedef struct {
  lmn_word   volatile timestamp;
  lmn_word   volatile lock;
} lmn_hopch_segment_t;

#define LMN_HOPCH_RAHGE             64
#define LMN_HOPCH_INSERT_RANGE      4096
#define LMN_HOPCH_RESIZE_FACTOR     2

#define LMN_HOPCH_CACHE_LINE_SIZE   64
#define LMN_HOPCH_CACHE_MASK        ((LMN_HOPCH_CACHE_LINE_SIZE / sizeof(lmn_hopch_bucket_t)) - 1)

typedef struct {
  lmn_word              volatile segment_shift;
  lmn_word              volatile segment_mask;
  lmn_word              volatile bucket_mask;
  lmn_hopch_segment_t*  volatile segments;
  lmn_hopch_bucket_t*   volatile buckets;
} lmn_hopch_hashmap_t;

void        lmn_hopch_hashmap_init(lmn_hopch_hashmap_t*);
lmn_data_t  lmn_hopch_hashmap_put(lmn_hopch_hashmap_t*, const lmn_key_t, const lmn_data_t);
lmn_data_t  lmn_hopch_hashmap_find(const lmn_hopch_hashmap_t*, const lmn_key_t);

#endif /* ifndef LMN_HOPSCOTCH_HASHMAP_H */


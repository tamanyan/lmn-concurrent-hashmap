/**
 * License GPL3
 * @file   lmn_hopscotch_hashmap.h
 * @brief  hopscotch hash table implementation
 * @author Taketo Yoshida
*/
#ifndef LMN_HOPSCOTCH_HASHMAP_H
#  define LMN_HOPSCOTCH_HASHMAP_H

#include "lmn_map_util.h"

typedef struct {
  unsigned long    volatile hop_info;
  unsigned long   volatile hash;
  lmn_key_t       volatile key;
  lmn_data_t      volatile data;
} lmn_hopch_bucket_t;

typedef struct {
  unsigned int    volatile timestamp;
  unsigned int    volatile lock;
} lmn_hopch_segment_t;

#define LMN_HOPCH_RAHGE           64
#define LMN_HOPCH_INSERT_RANGE    4096
#define LMN_HOPCH_RESIZE_FACTOR   2

typedef struct {
  unsigned int          volatile segment_shift;
  unsigned int          volatile segment_mask;
  unsigned int          volatile bucket_mask;
  lmn_hopch_segment_t*  volatile segments;
  lmn_hopch_bucket_t*   volatile buckets;
} lmn_hopch_hashmap_t;

#endif /* ifndef LMN_HOPSCOTCH_HASHMAP_H */


/**
 * @file   lmn_closed_hashmap.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef LMN_CLOSED_HASHMAP_H
#  define LMN_CLOSED_HASHMAP_H

#include "lmn_hash.h"
#include "lmn_map_util.h"

typedef struct {
  lmn_word      volatile hash;
  lmn_key_t     volatile key;
  lmn_data_t    volatile data;
} lmn_closed_bucket_t;

typedef struct {
  lmn_word               volatile bucket_mask;
  lmn_word               volatile size;
  lmn_closed_bucket_t**  volatile buckets;
} lmn_closed_hashmap_t;

void lmn_closed_init(lmn_closed_hashmap_t* map);
lmn_data_t lmn_closed_find(lmn_closed_hashmap_t *map, lmn_key_t key);
void lmn_closed_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data);

#endif /* ifndef LMN_CLOSED_HASHMAP_H */


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
} lmn_closed_entry_t;

typedef struct {
  lmn_word               volatile bucket_mask;
  lmn_word               volatile size;
  lmn_closed_entry_t**   volatile tbl;
} lmn_closed_hashmap_t;

#define lmn_closed_foreach(map, ent, code) \
  {                                                           \
    int __i;                                                  \
    for (__i = 0; __i < ((map)->bucket_mask + 1); __i++) {    \
      (ent) = (map)->tbl[__i];                                \
      if ((ent) != LMN_HASH_EMPTY) {                          \
        (code);                                               \
      }                                                       \
    }                                                         \
  }

#define lmn_closed_free(map, ent, code) \
  {                                      \
    lmn_closed_foreach(map, ent, code);  \
    lmn_free((map)->tbl);                \
  }

void lmn_closed_init(lmn_closed_hashmap_t* map);
lmn_data_t lmn_closed_find(lmn_closed_hashmap_t *map, lmn_key_t key);
void lmn_closed_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data);
void lmn_closed_free_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data);

#endif /* ifndef LMN_CLOSED_HASHMAP_H */


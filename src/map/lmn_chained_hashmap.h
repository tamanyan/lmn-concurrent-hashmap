/**
 * @file   lmn_chained_hashmap.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef LMN_CHAINED_HASHMAP_H
#  define LMN_CHAINED_HASHMAP_H

#include "lmn_hash.h"
#include "lmn_map_util.h"

typedef struct _lmn_chained_entry_t {
  lmn_key_t                    volatile key;
  lmn_data_t                   volatile data;
  struct _lmn_chained_entry_t* volatile next;
} lmn_chained_entry_t;

typedef struct {
  lmn_word               volatile bucket_mask;
  lmn_word               volatile size;
  lmn_chained_entry_t**  volatile tbl;
  pthread_mutex_t        mutexs[HASHMAP_SEGMENT];
} lmn_chained_hashmap_t;

#define lmn_chained_foreach(map, ent, code)             \
  {                                                           \
    int __i;                                                  \
    lmn_chained_entry_t *__next;                              \
    for (__i = 0; __i < ((map)->bucket_mask + 1); __i++) {    \
      (ent) = (map)->tbl[__i];                                \
      while ((ent) != LMN_HASH_EMPTY) {                       \
        __next = (ent)->next;                                 \
        (code);                                               \
        (ent) = __next;                                       \
      }                                                       \
    }                                                         \
  }

#define lmn_chained_free(map, ent, code) \
  {                                       \
    lmn_chained_foreach(map, ent, code);  \
    lmn_free((map)->tbl);                 \                          
  }


void lmn_chained_init(lmn_chained_hashmap_t* map);
lmn_data_t lmn_chained_find(lmn_chained_hashmap_t *map, lmn_key_t key);
void lmn_chained_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data);
lmn_data_t lmn_chained_free_find(lmn_chained_hashmap_t *map, lmn_key_t key);
void lmn_chained_free_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data);

#endif /* ifndef LMN_CHAINED_HASHMAP_H */


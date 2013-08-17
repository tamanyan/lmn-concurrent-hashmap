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
  lmn_word                     volatile hash;
  lmn_key_t                    volatile key;
  lmn_data_t                   volatile data;
  struct _lmn_chained_entry_t* volatile next;
} lmn_chained_entry_t;

typedef struct {
  lmn_word               volatile bucket_mask;
  lmn_word               volatile size;
  lmn_chained_entry_t**  volatile tbl;
} lmn_chained_hashmap_t;

void lmn_chained_init(lmn_chained_hashmap_t* map);
lmn_data_t lmn_chained_find(lmn_chained_hashmap_t *map, lmn_key_t key);
void lmn_chained_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data);

#endif /* ifndef LMN_CHAINED_HASHMAP_H */

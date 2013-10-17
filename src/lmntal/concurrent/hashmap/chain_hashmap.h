/**
 * @file   chain_hashmap.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef CHAIN_HASHMAP_H
#  define CHAIN_HASHMAP_H

#include "hashmap.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

typedef struct _chain_entry_t {
  lmn_word               volatile key;
  lmn_data_t             volatile data;
  struct _chain_entry_t* volatile next;
} chain_entry_t;

typedef struct {
  lmn_word         volatile bucket_mask;
  lmn_word         volatile size;
  chain_entry_t**  volatile tbl;
  int              volatile resize;
  pthread_mutex_t        mutexs[HASHMAP_SEGMENT];
} chain_hashmap_t;

typedef struct {
  lmn_word         volatile bucket_mask;
  lmn_word         volatile size;
  chain_entry_t**  volatile tbl;
} lf_chain_hashmap_t;

void chain_init(chain_hashmap_t* map);
lmn_data_t chain_find(chain_hashmap_t *map, lmn_key_t key);
void chain_put(chain_hashmap_t *map, lmn_key_t key, lmn_data_t data);
void chain_free(chain_hashmap_t* map);

}
}
}

#endif /* ifndef CHAIN_HASHMAP_H */


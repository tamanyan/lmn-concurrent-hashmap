/**
 * @file   lf_chain_hashmap.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "lf_chain_hashmap.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

/*
 * public functions
 */

void lf_chain_init(chain_hashmap_t* map) {
  map->tbl              = (chain_entry_t**)calloc(sizeof(chain_entry_t*), LMN_DEFAULT_SIZE);
  map->bucket_mask      = LMN_DEFAULT_SIZE- 1;
  map->size             = 0;
  memset(map->tbl, 0x00, LMN_DEFAULT_SIZE);
}

lmn_data_t lf_chain_find(chain_hashmap_t *map, lmn_key_t key) {
  lmn_word bucket     = hash<lmn_word>(key) & map->bucket_mask;
  chain_entry_t *ent  = map->tbl[bucket];
  chain_entry_t *old;

  while(ent != LMN_HASH_EMPTY) {
    if (ent->key == key) {
      return ent->data;
    }
    ent = ent->next;
  }
  return NULL;
}

void lf_chain_free(chain_hashmap_t* map) {

}

void lf_chain_put(chain_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_word bucket        = hash<lmn_word>(key) & map->bucket_mask;
  chain_entry_t **ent    = &map->tbl[bucket];

  if ((*ent) == LMN_HASH_EMPTY) {
    chain_entry_t *tmp, *new_ent = NULL;
    do {
      tmp = *ent;
      if (new_ent == NULL) {
        new_ent = (chain_entry_t*)malloc(sizeof(chain_entry_t));
        new_ent->next = NULL;
      } else {
        new_ent->next = (*ent);
      }
    } while(!LMN_CAS(&(*ent), tmp, new_ent));
  } else {
    chain_entry_t *cur, *tmp, *new_ent = NULL;
    cur = *ent;
    do {
      tmp = *ent;
      do {
        if (cur->key == key) {
          cur->data = data;
          if (new_ent == NULL) free(new_ent);
          return;
        }
        //printf("%p ", cur->next);
      } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
      if (new_ent == NULL)
        new_ent = (chain_entry_t*)malloc(sizeof(chain_entry_t));
      new_ent->next = (*ent);
    } while(!LMN_CAS(&(*ent), tmp, new_ent));
    // dbgprint("insert key:%d, data:%d\n", key, data);
  }
  (*ent)->key  = key;
  (*ent)->data = data;
  LMN_ATOMIC_ADD(&(map->size), 1);
}

}
}
}


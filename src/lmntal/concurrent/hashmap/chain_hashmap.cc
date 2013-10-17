/**
 * @file   chain_hashmap.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "chain_hashmap.h"
#include "../thread.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

void chain_rehash(chain_hashmap_t *map) {
  int i;
  lmn_word                new_size = map->bucket_mask + 1;
  lmn_word                old_size = new_size;
  chain_entry_t    **new_tbl = lmn_calloc(chain_entry_t*, new_size <<= 2);
  chain_entry_t    **old_tbl = map->tbl;
  chain_entry_t  *ent, *next;
  lmn_word               bucket;
  lmn_word         new_bucket_mask = new_size - 1;

  //printf("rehash start count:%d old_size:%d new_size:%d\n", map->size, old_size, new_size);
  for (i = 0; i < old_size; i++) {
    ent = old_tbl[i];
    while (ent) {
      next = ent->next;
      bucket = (hash<lmn_word>(ent->key) & new_bucket_mask);
      ent->next = new_tbl[bucket];
      new_tbl[bucket] = ent;
      ent = next;
    }
  }

  map->bucket_mask = new_bucket_mask;
  map->tbl         = new_tbl;
  lmn_free(old_tbl);
}
/*
 * public functions
 */

void chain_init(chain_hashmap_t* map) {
  map->tbl              = (chain_entry_t**)calloc(sizeof(chain_entry_t*), LMN_DEFAULT_SIZE);
  map->bucket_mask      = LMN_DEFAULT_SIZE- 1;
  map->size             = 0;
  memset(map->tbl, 0x00, LMN_DEFAULT_SIZE);
  {
    pthread_mutexattr_t mattr[HASHMAP_SEGMENT];
    int i;
    for (i = 0; i < HASHMAP_SEGMENT; i++) {
      pthread_mutexattr_init(&mattr[i]);
      pthread_mutexattr_setpshared(&mattr[i], PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&map->mutexs[i], &mattr[i]);
    }
  }
}

void chain_free(chain_hashmap_t* map) {

}

lmn_data_t chain_find(chain_hashmap_t *map, lmn_key_t key) {
  lmn_word bucket     = hash<lmn_word>(key) & map->bucket_mask;

  pthread_mutex_lock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
  lmn_word new_bucket    = hash<lmn_word>(key) & map->bucket_mask;
  if (new_bucket != bucket) {
    pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
    pthread_mutex_lock(&map->mutexs[new_bucket % HASHMAP_SEGMENT]);
    bucket = new_bucket;
  }
  chain_entry_t *ent  = map->tbl[bucket];
  while(ent != LMN_HASH_EMPTY) {
    if (ent->key == key) {
      pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
      return ent->data;
    }
    ent = ent->next;
  }
  pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
  return NULL;
}

void chain_put(chain_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_word bucket        = hash<lmn_word>(key) & map->bucket_mask;
  chain_entry_t *cur, *tmp;

  pthread_mutex_lock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
  lmn_word new_bucket    = hash<lmn_word>(key) & map->bucket_mask;
  if (new_bucket != bucket) {
    pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
    pthread_mutex_lock(&map->mutexs[new_bucket % HASHMAP_SEGMENT]);
    bucket = new_bucket;
  }
  chain_entry_t **ent    = &map->tbl[bucket];
  if ((*ent) == LMN_HASH_EMPTY) {
    (*ent) = lmn_malloc(chain_entry_t);
    (*ent)->next = NULL;
    //dbgprint("insert new key:%d, data:%d\n", key, data);
  } else {
    cur = *ent;
    do {
      if (cur->key == key) {
        cur->data = data;
        pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
        return;
      }
      //printf("%p ", cur->next);
    } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
    cur = lmn_malloc(chain_entry_t);
    cur->next = (*ent);
    (*ent) = cur;
    // dbgprint("insert key:%d, data:%d\n", key, data);
  }
  (*ent)->key  = key;
  (*ent)->data = data;
  LMN_ATOMIC_ADD(&map->size, 1);
  pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
  if (map->size > map->bucket_mask * 0.75) {
    if (!map->resize && LMN_CAS(&map->resize, 0, 1)) {
      for(int i = 0; i < HASHMAP_SEGMENT; i++) {
        pthread_mutex_lock(&map->mutexs[i]);
      }
      chain_rehash(map);
      for(int i = 0; i < HASHMAP_SEGMENT; i++) {
        pthread_mutex_unlock(&map->mutexs[i]);
      }
      map->resize = 0;
    }
  }
}

}
}
}

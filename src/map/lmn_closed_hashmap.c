/**
 * License GPL3
 * @file   lmn_closed_hashmap.c
 * @brief  
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "lmn_closed_hashmap.h"

#define LMN_CLOSED_INIT_SIZE (64 * 1024)

/*
 * private functions
 */
lmn_hash_t lmn_closed_find_bucket(lmn_closed_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t i             = lmn_hash_calc(key);
  lmn_closed_entry_t **tbl = map->tbl;
  lmn_word mask            = map->bucket_mask;
  while (tbl[i & mask] != LMN_HASH_EMPTY && tbl[i & mask]->key != key) {
    i = (i + 1);
  }
  return i & mask;
}

void lmn_closed_rehash(lmn_closed_hashmap_t *map) {
  int i;
  lmn_word            new_size = map->bucket_mask + 1;
  lmn_word            old_size = new_size;
  lmn_closed_entry_t **new_tbl = lmn_malloc(new_size <<= 2, lmn_closed_entry_t*);
  lmn_word                hash;
  lmn_closed_entry_t **old_tbl = map->tbl;
  lmn_closed_hashmap_t tmp_map;

  tmp_map.tbl = new_tbl;
  tmp_map.bucket_mask = new_size - 1;

  dbgprint("rehash start\n", NULL);
  for (i = 0; i < old_size; i++) {
    lmn_closed_entry_t *entry = old_tbl[i];
    if (entry != LMN_HASH_EMPTY) {
      hash          = lmn_closed_find_bucket(&tmp_map, entry->key);
      entry->hash   = hash;
      new_tbl[hash] = entry;
    }
  }
  map->bucket_mask = tmp_map.bucket_mask;
  map->tbl         = new_tbl;
  lmn_free(old_tbl);
}

/*
 * public functions
 */
void lmn_closed_init(lmn_closed_hashmap_t *map) {
  //map->tbl         = (lmn_closed_entry_t**)malloc(sizeof(lmn_closed_entry_t*) * LMN_CLOSED_INIT_SIZE);
  map->tbl          = lmn_malloc(LMN_CLOSED_INIT_SIZE, lmn_closed_entry_t*);
  map->bucket_mask  = LMN_CLOSED_INIT_SIZE - 1;
  map->size         = LMN_CLOSED_INIT_SIZE;
  memset(map->tbl, 0x00, LMN_CLOSED_INIT_SIZE);
  //dbgprint("init closed hash map %p\n", map);
}

lmn_data_t lmn_closed_find(lmn_closed_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t             hash = lmn_closed_find_bucket(map, key);
  lmn_closed_entry_t *entry = map->tbl[hash];

  if (entry != LMN_HASH_EMPTY) {
    return entry->data;
  } else {
    return LMN_HASH_EMPTY_DATA;
  }
}

void lmn_closed_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_hash_t           hash = lmn_closed_find_bucket(map, key);
  lmn_closed_entry_t **entry = &map->tbl[hash];
  //dbgprint("put %lu %lu\n", key, data);
  if ((*entry) != LMN_HASH_EMPTY) {
    (*entry)->data = data;
  } else {
    (*entry) = lmn_malloc(1, lmn_closed_entry_t);
    (*entry)->key  = key;
    (*entry)->hash = hash;
    (*entry)->data = data;
    map->size++;
    if (map->size > map->bucket_mask * 0.75) {
      lmn_closed_rehash(map);
    }
  }
}

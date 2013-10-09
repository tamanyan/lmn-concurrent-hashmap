/**
 * @file   lmn_closed_hashmap.c
 * @brief  
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "lmn_closed_hashmap.h"

#define LMN_CLOSED_INIT_SIZE (32 * 1024 * 1024)

inline lmn_hash_t lmn_hash_calc(lmn_key_t key) {
#ifdef __x86_64__
  /* Robert Jenkins' 32 bit Mix Function */
  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);

  /* Knuth's Multiplicative Method */
  key = (key >> 3) * 2654435761;
#else
  key ^= (key << 15) ^ 0xcd7dcd7d;
  key ^= (key >> 10);
  key ^= (key <<  3);
  key ^= (key >>  6);
  key ^= (key <<  2) + (key << 14);
  key ^= (key >> 16);
#endif
  return key;
}

inline int lmn_hash_eq(lmn_hash_t h1, lmn_hash_t h2) {
  return h1 == h2;
}

inline int lmn_hash_key_eq(lmn_hash_t key1, lmn_hash_t key2) {
  return key1 == key2;
}

/*
 * private functions
 */
inline lmn_hash_t lmn_closed_find_bucket(lmn_closed_hashmap_t *map, lmn_key_t key) {
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
  lmn_closed_entry_t **new_tbl = lmn_calloc(new_size <<= 2, lmn_closed_entry_t*);
  lmn_word                hash;
  lmn_closed_entry_t **old_tbl = map->tbl;
  lmn_closed_hashmap_t tmp_map;
  lmn_closed_entry_t *ent;

  tmp_map.tbl = new_tbl;
  tmp_map.bucket_mask = new_size - 1;

  dbgprint("rehash start\n", NULL);
  for (i = 0; i < old_size; i++) {
    ent = old_tbl[i];
    if (ent != LMN_HASH_EMPTY) {
      hash          = lmn_closed_find_bucket(&tmp_map, ent->key);
      new_tbl[hash] = ent;
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
  map->tbl          = lmn_calloc(LMN_CLOSED_INIT_SIZE, lmn_closed_entry_t*);
  map->bucket_mask  = LMN_CLOSED_INIT_SIZE - 1;
  map->size         = 0;
  memset(map->tbl, 0x00, LMN_CLOSED_INIT_SIZE);
  //dbgprint("init closed hash map %p\n", map);
}

lmn_data_t lmn_closed_find(lmn_closed_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t             hash = lmn_closed_find_bucket(map, key);
  lmn_closed_entry_t *ent = map->tbl[hash];

  if (ent != LMN_HASH_EMPTY) {
    return ent->data;
  } else {
    return LMN_HASH_EMPTY_DATA;
  }
}

void lmn_closed_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_hash_t        bucket = lmn_closed_find_bucket(map, key);
  lmn_closed_entry_t **ent = &map->tbl[bucket];
  //dbgprint("put %lu %lu\n", key, data);
  if ((*ent) != LMN_HASH_EMPTY) {
    (*ent)->data = data;
  } else {
    (*ent) = lmn_malloc(1, lmn_closed_entry_t);
    (*ent)->key  = key;
    (*ent)->data = data;
    LMN_ATOMIC_ADD(&(map->size), 1);
    if (map->size > map->bucket_mask * 0.75) {
      lmn_closed_rehash(map);
    }
  }
}

void lmn_closed_free_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_hash_t        bucket = lmn_closed_find_bucket(map, key);
  lmn_closed_entry_t **ent = &map->tbl[bucket];
  lmn_closed_entry_t *new_ent;
  //dbgprint("put %lu %lu\n", key, data);
  if ((*ent) != LMN_HASH_EMPTY) {
    (*ent)->data = data;
  } else {
    new_ent = lmn_malloc(1, lmn_closed_entry_t);
    if (!LMN_CAS(&(*ent), NULL, new_ent)) {
      lmn_free(new_ent);
      lmn_closed_free_put(map, key, data);
      return ;
    }
    (*ent)->key  = key;
    (*ent)->data = data;
    LMN_ATOMIC_ADD(&(map->size), 1);
    if (map->size > map->bucket_mask * 0.75) {
      lmn_closed_rehash(map);
    }
  }
}

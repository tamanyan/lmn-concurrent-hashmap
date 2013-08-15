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
lmn_hash_t lmn_closed_find_hash(lmn_closed_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t i              = lmn_hash_calc(key);
  lmn_closed_bucket_t **tbl = map->buckets;
  lmn_word mask             = map->bucket_mask;
  while (tbl[i & mask] != LMN_HASH_EMPTY && tbl[i & mask]->key != key) {
    i = (i + 1);
  }
  return i & mask;
}

void lmn_closed_rehash(lmn_closed_hashmap_t *map) {
  int i;
  lmn_word             new_size = map->bucket_mask + 1;
  lmn_word             old_size = new_size;
  lmn_closed_bucket_t **new_tbl = lmn_malloc(new_size <<= 2, lmn_closed_bucket_t*);
  lmn_word                 hash;
  lmn_closed_bucket_t **old_tbl = map->buckets;
  lmn_closed_hashmap_t  tmp_map;

  tmp_map.buckets = new_tbl;
  tmp_map.bucket_mask = new_size - 1;

  dbgprint("rehash start\n", NULL);
  for (i = 0; i < old_size; i++) {
    lmn_closed_bucket_t *bucket = old_tbl[i];
    if (bucket != LMN_HASH_EMPTY) {
      hash = lmn_closed_find_hash(&tmp_map, bucket->key);
      bucket->hash = hash;
      new_tbl[hash] = bucket;
    }
  }
  map->bucket_mask = tmp_map.bucket_mask;
  map->buckets     = new_tbl;
  lmn_free(old_tbl);
}

/*
 * public functions
 */
void lmn_closed_init(lmn_closed_hashmap_t *map) {
  //map->buckets          = (lmn_closed_bucket_t**)malloc(sizeof(lmn_closed_bucket_t*) * LMN_CLOSED_INIT_SIZE);
  map->buckets          = lmn_malloc(LMN_CLOSED_INIT_SIZE, lmn_closed_bucket_t*);
  map->bucket_mask      = LMN_CLOSED_INIT_SIZE - 1;
  map->size             = LMN_CLOSED_INIT_SIZE;
  memset(map->buckets, 0x00, LMN_CLOSED_INIT_SIZE);
  //dbgprint("init closed hash map %p\n", map);
}

lmn_data_t lmn_closed_find(lmn_closed_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t             hash = lmn_closed_find_hash(map, key);
  lmn_closed_bucket_t *bucket = map->buckets[hash];

  if (bucket != LMN_HASH_EMPTY) {
    return bucket->data;
  } else {
    return LMN_HASH_EMPTY_DATA;
  }
}

void lmn_closed_put(lmn_closed_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_hash_t              hash = lmn_closed_find_hash(map, key);
  lmn_closed_bucket_t **bucket = &map->buckets[hash];
  //dbgprint("put %lu %lu\n", key, data);
  if ((*bucket) != LMN_HASH_EMPTY) {
    (*bucket)->data = data;
  } else {
    (*bucket) = lmn_malloc(1, lmn_closed_bucket_t);
    (*bucket)->key  = key;
    (*bucket)->hash = hash;
    (*bucket)->data = data;
    map->size++;
    if (map->size > map->bucket_mask * 0.75) {
      lmn_closed_rehash(map);
    }
  }
}

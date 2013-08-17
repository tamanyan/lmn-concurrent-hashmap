/**
 * @file   lmn_chained_hashmap.c
 * @brief  
 * @author Taketo Yoshida
 */
#include "lmn_chained_hashmap.h"

#define LMN_CHAINED_INIT_SIZE (32 * 1024 * 1024)

/*
 * private functions
 */

void lmn_chained_print_map(lmn_chained_hashmap_t *map) {
  lmn_chained_entry_t *ent;
  int i;
  int count = 0;
  dbgprint("======================================================================\n", NULL);
  for (i = 0; i < (map->bucket_mask + 1); i++) {
    ent = map->tbl[i];
    dbgprint("(%d) ",i);
    while (ent != LMN_HASH_EMPTY) {
      dbgprint("%x:%d:%d ", ent->hash, ent->key, ent->data);
      count++;
      ent = ent->next;
    }
    dbgprint("\n", NULL);
  }
  dbgprint("tbl size %d, count %d org_count %d\n", map->bucket_mask+1, count, map->size);
  dbgprint("======================================================================\n", NULL);
}

void lmn_chained_rehash(lmn_chained_hashmap_t *map) {
  int i;
  lmn_word                new_size = map->bucket_mask + 1;
  lmn_word                old_size = new_size;
  lmn_chained_entry_t    **new_tbl = lmn_calloc(new_size <<= 2, lmn_chained_entry_t*);
  lmn_chained_entry_t    **old_tbl = map->tbl;
  lmn_chained_entry_t  *ent, *next;
  lmn_hash_t                bucket;
  lmn_word         new_bucket_mask = new_size - 1;

  dbgprint("rehash start old_size:%d\n", old_size);
  // lmn_chained_print_map(map);
  for (i = 0; i < old_size; i++) {
    ent = old_tbl[i];
    while (ent) {
      next = ent->next;
      bucket = (lmn_hash_calc(ent->key) & new_bucket_mask);
      ent->next = new_tbl[bucket];
      new_tbl[bucket] = ent;
      ent = next;
    }
  }
  map->bucket_mask = new_bucket_mask;
  map->tbl         = new_tbl;
  //lmn_chained_print_map(map);
  lmn_free(old_tbl);
}


/*
 * public functions
 */

void lmn_chained_init(lmn_chained_hashmap_t* map) {
  map->tbl              = lmn_calloc(LMN_CHAINED_INIT_SIZE, lmn_chained_entry_t*);
  map->bucket_mask      = LMN_CHAINED_INIT_SIZE - 1;
  map->size             = 0;
  memset(map->tbl, 0x00, LMN_CHAINED_INIT_SIZE);
}


lmn_data_t lmn_chained_find(lmn_chained_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t hash           = lmn_hash_calc(key);
  lmn_chained_entry_t *ent  = map->tbl[hash & map->bucket_mask];

  while(ent != LMN_HASH_EMPTY) {
    if (ent->key == key) {
      return ent->data;
    }
    ent = ent->next;
  }
  return NULL;
}

void lmn_chained_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_hash_t hash              = lmn_hash_calc(key);
  lmn_chained_entry_t **ent    = &map->tbl[hash & map->bucket_mask];
  lmn_chained_entry_t *cur;

  if ((*ent) == LMN_HASH_EMPTY) {
    (*ent) = lmn_malloc(1, lmn_chained_entry_t);
    // dbgprint("insert new key:%d, data:%d\n", key, data);
  } else {
    cur = *ent;
    do {
      if (cur->key == key) {
        cur->data = data;
        return;
      }
      //printf("%p ", cur->next);
    } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
    cur->next = lmn_malloc(1, lmn_chained_entry_t);
    ent = &(cur->next);
    // dbgprint("insert key:%d, data:%d\n", key, data);
  }
  (*ent)->next = NULL;
  (*ent)->key  = key;
  (*ent)->hash = hash;
  (*ent)->data = data;
  map->size++;
  if (map->size > map->bucket_mask * 0.75) {
    lmn_chained_rehash(map);
  }
}

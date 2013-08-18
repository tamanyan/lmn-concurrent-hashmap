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
      dbgprint("%d:%d ", ent->key, ent->data);
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
# ifndef HASHMAP_LOCK_FREE
  {
    pthread_mutexattr_t mattr[HASHMAP_SEGMENT];
    int i;
    for (i = 0; i < HASHMAP_SEGMENT; i++) {
      pthread_mutexattr_init(&mattr[i]);
      pthread_mutexattr_setpshared(&mattr[i], PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&map->mutexs[i], &mattr[i]);
    }
  }
# endif
}

lmn_data_t lmn_chained_find(lmn_chained_hashmap_t *map, lmn_key_t key) {
  lmn_word bucket           = lmn_hash_calc(key) & map->bucket_mask;
  lmn_chained_entry_t *ent  = map->tbl[bucket];

# ifndef HASHMAP_LOCK_FREE
  pthread_mutex_lock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
# endif
  while(ent != LMN_HASH_EMPTY) {
    if (ent->key == key) {
# ifndef HASHMAP_LOCK_FREE
      pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
# endif
      return ent->data;
    }
    ent = ent->next;
  }
# ifndef HASHMAP_LOCK_FREE
  pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
# endif
  dbgprint("not found data key:%d\n", key);
  return NULL;
}

void lmn_chained_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_word bucket              = lmn_hash_calc(key) & map->bucket_mask;
  lmn_chained_entry_t **ent    = &map->tbl[bucket];
  lmn_chained_entry_t *cur, *tmp;

# ifndef HASHMAP_LOCK_FREE
  pthread_mutex_lock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
# endif
  if ((*ent) == LMN_HASH_EMPTY) {
    (*ent) = lmn_malloc(1, lmn_chained_entry_t);
    (*ent)->next = NULL;
    //dbgprint("insert new key:%d, data:%d\n", key, data);
  } else {
    cur = *ent;
    do {
      if (cur->key == key) {
        cur->data = data;
# ifndef HASHMAP_LOCK_FREE
        pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
# endif
        return;
      }
      //printf("%p ", cur->next);
    } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
    cur = lmn_malloc(1, lmn_chained_entry_t);
    cur->next = (*ent);
    (*ent) = cur;
    // dbgprint("insert key:%d, data:%d\n", key, data);
  }
  (*ent)->key  = key;
  (*ent)->data = data;
  LMN_ATOMIC_ADD(&(map->size), 1);
  // map->size++;
  if (map->size > map->bucket_mask * 0.75) {
    lmn_chained_rehash(map);
  }
# ifndef HASHMAP_LOCK_FREE
  pthread_mutex_unlock(&map->mutexs[bucket % HASHMAP_SEGMENT]);
# endif
}

lmn_data_t lmn_chained_free_find(lmn_chained_hashmap_t *map, lmn_key_t key) {
  lmn_word bucket           = lmn_hash_calc(key) & map->bucket_mask;
  lmn_chained_entry_t *ent  = map->tbl[bucket];
  lmn_chained_entry_t *old;

  while(ent != LMN_HASH_EMPTY) {
    if (ent->key == key) {
      return ent->data;
    }
    ent = ent->next;
  }
  dbgprint("not found data key:%d\n", key);
  return NULL;
}

void lmn_chained_free_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_word bucket              = lmn_hash_calc(key) & map->bucket_mask;
  lmn_chained_entry_t **ent    = &map->tbl[bucket];

  if ((*ent) == LMN_HASH_EMPTY) {
    (*ent) = lmn_malloc(1, lmn_chained_entry_t);
    (*ent)->next = NULL;
    //dbgprint("insert new key:%d, data:%d\n", key, data);
  } else {
    lmn_chained_entry_t *cur, *tmp, *new_ent = NULL;
    cur = *ent;
    do {
      tmp = *ent;
      do {
        if (cur->key == key) {
          cur->data = data;
          if (new_ent == NULL) lmn_free(new_ent)
          return;
        }
        //printf("%p ", cur->next);
      } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
      if (new_ent == NULL)
        new_ent = lmn_malloc(1, lmn_chained_entry_t);
      new_ent->next = (*ent);
    } while(!LMN_CAS(&(*ent), tmp, new_ent));
    // dbgprint("insert key:%d, data:%d\n", key, data);
  }
  (*ent)->key  = key;
  (*ent)->data = data;
  LMN_ATOMIC_ADD(&(map->size), 1);
  // map->size++;
  if (map->size > map->bucket_mask * 0.75) {
    lmn_chained_rehash(map);
  }
}

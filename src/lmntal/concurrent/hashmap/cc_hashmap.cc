/**
 * @file   cc_hashmap.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "cc_hashmap.h"
#include "../thread.h"
#include <assert.h>

namespace lmntal {
namespace concurrent {
namespace hashmap {

using namespace lmntal::concurrent;

#define CC_CACHE_LINE_SIZE_FOR_UNIT64 8
#define CC_ENTRIES_PER_COPY_CHUNK 16
#define CC_PROB_FAIL -1
#define CC_INVALID_BUCKET -1
#define CC_COPIED_VALUE ((lmn_data_t)-2)
#define THRESHOLD 2

int call_count = 0;
int prob_count = 0;

/*
 * private functions
 */

__thread int count = 0;

inline void cc_hashmap_inc_count(cc_hashmap_t *map) {
  count++;
  //map->count[GetCurrentThreadId()]++;
}

inline lmn_word cc_hashmap_find_free_bucket(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word             offset = hash<lmn_word>(key);
  volatile lmn_key_t *buckets = map->buckets;
  lmn_word               mask = map->bucket_mask;
  int                   count = 0;
  if (LMN_UNLIKELY(key == CC_INVALID_BUCKET)) {
    fprintf(stderr, "find invalid key\n");
    assert(key != CC_INVALID_BUCKET);
  }

  //call_count++;
  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      //prob_count++;
      if (buckets[(offset + i) & mask] == CC_INVALID_BUCKET)  {
        return (offset + i) & mask;
      }
    }
    offset = hash<lmn_word>(offset);
    count++;
  }
  fprintf(stderr, "prob fail\n");
  return CC_PROB_FAIL;
}

inline lmn_word cc_hashmap_find_bucket(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word             offset = hash<lmn_word>(key);
  volatile lmn_key_t *buckets = map->buckets;
  lmn_word               mask = map->bucket_mask;
  int                   count = 0;
  if (LMN_UNLIKELY(key == CC_INVALID_BUCKET)) {
    fprintf(stderr, "find invalid key\n");
    assert(key != CC_INVALID_BUCKET);
  }

  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      if (buckets[(offset + i) & mask] == key)  {
        return (offset + i) & mask;
      } else if(buckets[(offset + i) & mask] == CC_INVALID_BUCKET) {
        return CC_PROB_FAIL;
      }
    }
    offset = hash<lmn_word>(offset);
    count++;
  }
  return CC_PROB_FAIL;
}

inline void cc_hashmap_init_inner(cc_hashmap_t *map, lmn_word scale) {
  static int thread_count = GetCurrentThreadCount();
  map->buckets      = lmn_calloc(lmn_key_t,  scale);
  map->data         = lmn_calloc(lmn_data_t, scale);
  map->bucket_mask  = scale - 1;
  map->count        = lmn_calloc(int, thread_count);
  memset((void*)map->buckets, CC_INVALID_BUCKET, sizeof(lmn_key_t)*scale);
}

static void cc_hashmap_start_copy(cc_hashmap_t *map) {
  int new_scale = (map->bucket_mask + 1) << 1;
  cc_hashmap_t *next = lmn_malloc(cc_hashmap_t);
  cc_hashmap_init_inner(next, new_scale);
  if (!LMN_CAS(&map->next, NULL, next)) {
    // another thread copy
    lmn_free(next);
  }
}

static int cc_hashmap_entry_copy(cc_hashmap_t *original, lmn_word index, lmn_word key_hash, cc_hashmap_t *replica) {

  return 0;
}

static int cc_hashmap_help_copy(cc_hashmap_t *map) {
  LMN_ASSERT(map->next != NULL);
  size_t limit;
  size_t total_copied = map->num_entries_copied;
  size_t num_copied = 0;
  size_t x = map->copy_scan;

  if (total_copied != cc_hashmap_tbl_size(map)) {
    // Panic if we've been around the array twice and still haven't finished the copy.
    int panic = (x >= cc_hashmap_tbl_size(map->next));
    if (!panic) {
      limit = CC_ENTRIES_PER_COPY_CHUNK;
      // Reserve some entries for this thread to copy. There is a race condition here because the
      // fetch and add isn't atomic, but that is ok.
      map->copy_scan = x + CC_ENTRIES_PER_COPY_CHUNK;

      // <copy_scan> might be larger than the size of the table, if some thread stalls while
      // copying. In that case we just wrap around to the begining and make another pass through
      // the table.
      x = (x & map->bucket_mask);
    } else {
      // scan the whole table
      x = 0;
      limit = cc_hashmap_tbl_size(map);
    }
    //printf("limit : %d, copy_scan:%d\n", limit, hti->copy_scan);

    // Copy the entries
    for (int i = 0; i < limit; ++i) {
      num_copied += cc_hashmap_entry_copy(map, x + i, 0, map->next);
      LMN_ASSERT(x >= cc_hashmap_tbl_size(map));
    }
    if (num_copied != 0) {
      total_copied = LMN_ATOMIC_ADD(&map->num_entries_copied, num_copied);
    }
  }

  return (total_copied == cc_hashmap_tbl_size(map));
}

lmn_data_t cc_hashmap_put_inner(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_word index = cc_hashmap_find_free_bucket(map, key);

  if (LMN_LIKELY(index != CC_PROB_FAIL && map->buckets[index] == CC_INVALID_BUCKET)) {
    if (!LMN_CAS(&map->buckets[index], CC_INVALID_BUCKET, data)) {
      //printf("insert fail retry key:%d, index:%d\n", key, index);
      return cc_hashmap_put_inner(map, key, data);
    } else {
      //printf("insert key:%d, index:%d\n", key, index);
      map->data[index] = data;
      //LMN_CAS(&(map->size), map->size, map->size + 1);
      //LMN_ATOMIC_ADD(&(map->size), 1);
      cc_hashmap_inc_count(map);
    }
  } else {
    //printf("insert fail key:%d, index:%d\n", key, index);
    if (map->next == NULL) {
      printf("start to copy\n");
      cc_hashmap_start_copy(map);
    }
    return CC_COPIED_VALUE;
  }
  return data;
}
/*
 * public function
 */
void cc_hashmap_init(cc_hashmap_t *map) {
  cc_hashmap_init_inner(map, LMN_DEFAULT_SIZE);
}

int cc_hashmap_count(cc_hashmap_t *map) {
  static int thread_count = GetCurrentThreadCount();
  int count = 0;
  for (int i = 0; i < thread_count; i++) {
    count += map->count[i];
  }
  return count;
}

void cc_hashmap_free(cc_hashmap_t *map) {
  lmn_free((void*)map->buckets);
  lmn_free((void*)map->data);
  if (map->count != NULL)
    lmn_free((void*)map->count);
}

lmn_data_t cc_hashmap_find(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word index = cc_hashmap_find_bucket(map, key);

  if (LMN_UNLIKELY(index != CC_PROB_FAIL)) {
    if (map->next != NULL) 
      cc_hashmap_find(map->next, key);
    return LMN_HASH_EMPTY_DATA;
  }
  lmn_data_t data = map->data[index]; 
  if (LMN_UNLIKELY(data == CC_COPIED_VALUE)) {
    data = cc_hashmap_find(map, key);
  }
  return data;
}

void cc_hashmap_put(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data) {

  if (LMN_UNLIKELY(map->next != NULL)) {
    cc_hashmap_help_copy(map);
  }

  while((map != NULL) && cc_hashmap_put_inner(map, key, data) == CC_COPIED_VALUE) {
    map = map->next;
  }
}

}
}
}

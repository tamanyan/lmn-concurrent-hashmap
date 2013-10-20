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

#define TAG1         (1ULL << 63)
#define TAG2         (1ULL << 62)

#define TAG_VALUE(v, tag) ((v) |  tag)
#define IS_TAGGED(v, tag) ((v) &  tag)
#define STRIP_TAG(v, tag) ((v) & ~tag)

#define CC_CACHE_LINE_SIZE_FOR_UNIT64 8
#define CC_ENTRIES_PER_COPY_CHUNK 16
#define CC_PROB_FAIL -1
#define CC_DOES_NOT_EXIST 0  // 000000...
#define CC_COPIED_VALUE ((lmn_data_t)(TAG_VALUE(CC_DOES_NOT_EXIST, TAG1))) // 100000...
#define THRESHOLD 3

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

inline lmn_word cc_hashmap_tbl_size(cc_hashmap_t *map) { return map->bucket_mask + 1; }

void cc_hashmap_print(cc_hashmap_t *map, int content) {
  LMN_DBG_V("addr:%p ", map);
  LMN_DBG_V("next:%p ", map->next);
  LMN_DBG_V("tbl-size:%d\n", cc_hashmap_tbl_size(map));
  LMN_DBG_V("copy_scan:%d ", map->copy_scan);
  LMN_DBG_V("num_entries_copied:%d\n", map->num_entries_copied);
  if (content) {
    for (int i = 0; i < 10; i++) {
      LMN_DBG_V("[%d]:[%d]\n", map->buckets[i], map->data[i]);
    }
  }
}

inline lmn_word cc_hashmap_find_free_bucket(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word             offset = hash<lmn_word>(key);
  volatile lmn_key_t *buckets = map->buckets;
  lmn_word               mask = map->bucket_mask;
  int                   count = 0;
  if (LMN_UNLIKELY(key == CC_DOES_NOT_EXIST)) {
    fprintf(stderr, "find invalid key\n");
    assert(key != CC_DOES_NOT_EXIST);
  }

  //call_count++;
  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      //prob_count++;
      if (buckets[(offset + i) & mask] == CC_DOES_NOT_EXIST)  {
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
  if (LMN_UNLIKELY(key == CC_DOES_NOT_EXIST)) {
    fprintf(stderr, "find invalid key\n");
    assert(key != CC_DOES_NOT_EXIST);
  }

  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      if (buckets[(offset + i) & mask] == key)  {
        return (offset + i) & mask;
      } else if(buckets[(offset + i) & mask] == CC_DOES_NOT_EXIST) {
        return CC_PROB_FAIL;
      }
    }
    offset = hash<lmn_word>(offset);
    count++;
  }
  return CC_PROB_FAIL;
}

inline lmn_word cc_hashmap_lookup(cc_hashmap_t *map, lmn_key_t key, int* is_empty) {
  lmn_word             offset = hash<lmn_word>(key);
  volatile lmn_key_t *buckets = map->buckets;
  lmn_word               mask = map->bucket_mask;
  int                   count = 0;
  if (LMN_UNLIKELY(key == CC_DOES_NOT_EXIST)) {
    fprintf(stderr, "find invalid key\n");
    assert(key != CC_DOES_NOT_EXIST);
  }

  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      if (buckets[(offset + i) & mask] == key)  {
        LMN_PTR_VAL(is_empty) = FALSE;
        return (offset + i) & mask;
      } else if(buckets[(offset + i) & mask] == CC_DOES_NOT_EXIST) {
        LMN_PTR_VAL(is_empty) = TRUE;
        return (offset + i) & mask;
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
  map->copy_scan    = 0;
  map->copy         = 0;
  map->num_entries_copied = 0;
}

static void cc_hashmap_start_copy(cc_hashmap_t *map) {
  if (map->copy == FALSE && LMN_CAS(&map->copy, FALSE, TRUE)) {
    LMN_DBG("[debug] cc_hashmap_start_copy: start to copy\n");
    cc_hashmap_print(map, 0);
    int new_scale = (map->bucket_mask + 1) << 1;
    cc_hashmap_t *next = lmn_malloc(cc_hashmap_t);
    cc_hashmap_init_inner(next, new_scale);
    if (!LMN_CAS(&map->next, NULL, next)) {
      // another thread copy
      lmn_free(next);
    }
  }
}

static int cc_hashmap_copy_entry(cc_hashmap_t *original, lmn_word index, lmn_word key_hash, cc_hashmap_t *replica) {

  // alive {null, null} or {null, X} or {K, null} or {K, X} or {K, V} or {K, V`}
  volatile lmn_data_t *org_data = (lmn_data_t*)&original->data[index];
  
  // already copied
  // {null, X} or {K, X}
  if (LMN_UNLIKELY(LMN_PTR_VAL(org_data) == CC_COPIED_VALUE)) {
    return FALSE;
  }
  // alive {null, null} or {K, null} or {K, V} or {K, V`}

  // empty 
  // {null, null} or {K, null}
  if (LMN_UNLIKELY(LMN_PTR_VAL(org_data) == CC_DOES_NOT_EXIST)) {
    if (LMN_CAS(org_data, CC_DOES_NOT_EXIST, CC_COPIED_VALUE)) {
      // {null, X} or {K, X}
      return TRUE;
    }
  }
  // alive {K, V} or {K, V`}

  LMN_PTR_VAL(org_data) = LMN_ATOMIC_AND_OR(org_data, TAG_VALUE(0, TAG1)); // add tag
  if (LMN_PTR_VAL(org_data) == CC_COPIED_VALUE) {
    return FALSE; // <value> was already copied by another thread.
  }
  // alive {K, V`}

  // Install the key in the new table.
  volatile lmn_key_t *org_key = (lmn_key_t*)&original->buckets[index];

  int rep_is_empty;
  lmn_word rep_index = cc_hashmap_lookup(replica, LMN_PTR_VAL(org_key), &rep_is_empty);
  if (rep_index == CC_PROB_FAIL) {
    fprintf(stderr, "prob fail at replica table\n");
    LMN_ASSERT(rep_index != CC_PROB_FAIL);
  }

  if (rep_is_empty) {
    if (!LMN_CAS(&replica->buckets[rep_index], CC_DOES_NOT_EXIST, LMN_PTR_VAL(org_key))) {
      //printf("retry copy entry\n");
      return cc_hashmap_copy_entry(original, index, key_hash, replica); // retry recursive tail-call
    }
  }

  LMN_PTR_VAL(org_data) = (lmn_data_t*)STRIP_TAG((lmn_word)LMN_PTR_VAL(org_data), TAG1); // remove tag

  int cas_succ = LMN_CAS(&replica->data[rep_index], CC_DOES_NOT_EXIST, LMN_PTR_VAL(org_data));

  if (replica->data[rep_index] == CC_COPIED_VALUE) {
    LMN_ASSERT(replica->data[rep_index] != CC_COPIED_VALUE);
  }

  LMN_PTR_VAL(org_data) = CC_COPIED_VALUE;

  if (cas_succ) {
    return TRUE;
  }
  return FALSE; // another thread completed the copy
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

    //printf("x:%d, tbl_size:%d\n", x, cc_hashmap_tbl_size(map));
    // Copy the entries
    for (int i = 0; i < limit; ++i) {
      num_copied += cc_hashmap_copy_entry(map, x + i, 0, map->next);
      LMN_ASSERT(x < cc_hashmap_tbl_size(map));
    }
    if (num_copied != 0) {
      total_copied = LMN_ATOMIC_ADD(&map->num_entries_copied, num_copied);
    }
  }

  // printf("%d\n", total_copied);
  return (total_copied == cc_hashmap_tbl_size(map));
}

lmn_data_t cc_hashmap_put_inner(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  int is_empty;
  lmn_word index = cc_hashmap_lookup(map, key, &is_empty);

  if (LMN_UNLIKELY(index == CC_PROB_FAIL)) {
    if (map->next == NULL) {
      cc_hashmap_start_copy(map);
    }
    return CC_COPIED_VALUE;
  }

  if (is_empty) {
    if (!LMN_CAS(&map->buckets[index], CC_DOES_NOT_EXIST, data)) {
      return cc_hashmap_put_inner(map, key, data); // retry
    }
    map->data[index] = data;
    //LMN_CAS(&(map->size), map->size, map->size + 1);
    cc_hashmap_inc_count(map);
  } else {
    // LMN_DBG("not empty\n");
    return CC_DOES_NOT_EXIST;
  }
  /*lmn_word index = cc_hashmap_find_free_bucket(map, key);

  if (LMN_LIKELY(index != CC_PROB_FAIL && map->buckets[index] == CC_DOES_NOT_EXIST)) {
    if (!LMN_CAS(&map->buckets[index], CC_DOES_NOT_EXIST, data)) {
      //printf("insert fail retry key:%d, index:%d\n", key, index);
      return cc_hashmap_put_inner(map, key, data);
    } else {
      //printf("insert key:%d, index:%d\n", key, index);
      map->data[index] = data;
      //LMN_CAS(&(map->size), map->size, map->size + 1);
      cc_hashmap_inc_count(map);
    }
  } else {
    //printf("insert fail key:%d, index:%d\n", key, index);
    if (map->next == NULL) {
      printf("start to copy\n");
      cc_hashmap_start_copy(map);
    }
    return CC_COPIED_VALUE;
  }*/
  return data;
}

lmn_data_t cc_hashmap_find_inner(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word index = cc_hashmap_find_bucket(map, key);

  if (index == CC_PROB_FAIL) {
    if (map->next != NULL) 
      return cc_hashmap_find_inner(map->next, key);
    return CC_DOES_NOT_EXIST;
  }
  lmn_data_t data = map->data[index]; 
  if (LMN_UNLIKELY(data == CC_COPIED_VALUE)) {
    data = cc_hashmap_find_inner(map->next, key);
  }
  return data;
}

/*
 * public function
 */

void lmn_hashmap_init(lmn_hashmap_t *lmn_map) {
  lmn_map->current = lmn_malloc(cc_hashmap_t);
  cc_hashmap_init_inner(lmn_map->current, 1 << 25);
}

int lmn_hashmap_count(lmn_hashmap_t *lmn_map) {
  cc_hashmap_t *map = lmn_map->current;
  static int thread_count = GetCurrentThreadCount();
  int count = 0;
  for (int i = 0; i < thread_count; i++) {
    count += map->count[i];
  }
  return count;
}

void lmn_hashmap_free(lmn_hashmap_t *lmn_map) {
  cc_hashmap_t *map = lmn_map->current;
  lmn_free((void*)map->buckets);
  lmn_free((void*)map->data);
  if (map->count != NULL)
    lmn_free((void*)map->count);
}

lmn_data_t lmn_hashmap_find(lmn_hashmap_t *lmn_map, lmn_key_t key) {
  cc_hashmap_t *map = lmn_map->current;
  return cc_hashmap_find_inner(map, key);
}

void lmn_hashmap_put(lmn_hashmap_t *lmn_map, lmn_key_t key, lmn_data_t data) {
  cc_hashmap_t *map = lmn_map->current;

  if (LMN_UNLIKELY(map->next != NULL)) {
    int done = cc_hashmap_help_copy(map);

    // Unlink fully copied tables.
    if (done) {
      LMN_ASSERT(map->next);
      if (LMN_CAS(&lmn_map->current, map, map->next)) {
        cc_hashmap_print(lmn_map->current, 0);
      }
    }
  }

  while((map != NULL) && cc_hashmap_put_inner(map, key, data) == CC_COPIED_VALUE) {
    map = map->next;
  }
}

}
}
}

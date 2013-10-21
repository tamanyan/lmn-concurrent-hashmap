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

#define CC_IMMUTABLE_FAIL ((lmn_data_t)-1)
#define CC_CACHE_LINE_SIZE_FOR_UNIT64 8
#define CC_ENTRIES_PER_COPY_CHUNK 16
#define CC_PROB_FAIL -1
#define CC_COPIED_VALUE ((lmn_data_t)(TAG_VALUE(CC_DOES_NOT_EXIST, TAG1))) // 100000...
#define THRESHOLD 4

#define CC_NEXT_SCALE(map) ((cc_hashmap_tbl_size(map) < (1 << 20)) ? (cc_hashmap_tbl_size(map) << 3) : (cc_hashmap_tbl_size(map) << 1))

int call_count = 0;
int prob_count = 0;

#include <execinfo.h>
#include <signal.h>

void stack_trace() {
    void *trace[128];
    int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
    backtrace_symbols_fd(trace, n, 1);
}

/*
 * private functions
 */

inline void cc_hashmap_inc_count(cc_hashmap_t *map) {
  //LMN_ATOMIC_ADD(&count, 1);
  map->count[GetCurrentThreadId()]++;
}

inline lmn_word cc_hashmap_tbl_size(cc_hashmap_t *map) { return map->bucket_mask + 1; }

inline int cc_hashmap_count(cc_hashmap_t *map) {
  static int thread_count = GetCurrentThreadCount();
  int count = 0;
  for (int i = 0; i < thread_count; i++) {
    count += map->count[i];
  }
  return count;
}

void cc_hashmap_print(cc_hashmap_t *map, int content) {
  LMN_DBG(
      "addr:%p next:%p tbl-size:%d\n copy_scan:%d num_entries_copied:%d\n", 
      map, 
      map->next,
      cc_hashmap_tbl_size(map),
      map->copy_scan,
      map->num_entries_copied
  );
  if (content) {
    for (int i = 0; i < 10; i++) {
      LMN_DBG("[%d]:[%d]\n", map->buckets[i], map->data[i]);
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
    stack_trace();
    assert(key != CC_DOES_NOT_EXIST);
  }

  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      if (buckets[(offset + i) & mask] == CC_DOES_NOT_EXIST) {
        LMN_PTR_VAL(is_empty) = TRUE;
        return (offset + i) & mask;
      } else if (buckets[(offset + i) & mask] == key)  {
        LMN_PTR_VAL(is_empty) = FALSE;
        return (offset + i) & mask;
      }
    }
    offset = hash<lmn_word>(offset);
    count++;
  }
  LMN_PTR_VAL(is_empty) = FALSE;
  return CC_PROB_FAIL;
}


inline void cc_hashmap_pool_init(cc_hashmap_t *map, lmn_word scale) {
  map->buckets      = lmn_calloc(lmn_key_t,  scale);
  map->data         = lmn_calloc(lmn_data_t, scale);
  map->bucket_mask  = scale - 1;
  map->copy_scan    = 0;
  map->copy         = 0;
  map->num_entries_copied = 0;
  map->count        = lmn_calloc(int, 100);
  map->pool         = lmn_malloc(cc_hashmap_t);
}

inline void cc_hashmap_pool_enable(cc_hashmap_t *pool) {
  cc_hashmap_pool_init(pool->pool, CC_NEXT_SCALE(pool));
}

inline void cc_hashmap_init_inner(cc_hashmap_t *map, lmn_word scale) {
  map->buckets      = lmn_calloc(lmn_key_t,  scale);
  map->data         = lmn_calloc(lmn_data_t, scale);
  map->bucket_mask  = scale - 1;
  map->count        = lmn_calloc(int, 100);
  map->copy_scan    = 0;
  map->copy         = 0;
  map->pool         = lmn_malloc(cc_hashmap_t);
  //cc_hashmap_pool_init(map->pool, (map->bucket_mask + 1) << 2);
  cc_hashmap_pool_init(map->pool, CC_NEXT_SCALE(map));
  map->num_entries_copied = 0;
}

static void cc_hashmap_start_copy(cc_hashmap_t *map) {
  //if (map->copy == FALSE && LMN_CAS(&map->copy, FALSE, TRUE)) {
  if (LMN_CAS(&map->next, NULL, map->pool)) {
    cc_hashmap_pool_enable(map->pool);
    map->pool = NULL;
    LMN_DBG("%s[debug] cc_hashmap_start_copy: start to copy, current:%d,next:%d %s\n", 
        LMN_TERMINAL_GREEN,
        cc_hashmap_tbl_size(map), cc_hashmap_tbl_size(map->next), 
        LMN_TERMINAL_DEFAULT);
  }
  //}
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

  // LMN_PTR_VAL(org_data) = LMN_ATOMIC_AND_OR(org_data, TAG_VALUE(0, TAG1)); // add tag
  if (LMN_PTR_VAL(org_data) == CC_COPIED_VALUE) {
    LMN_DBG("already copied {%u, %u}\n", original->buckets[index], LMN_PTR_VAL(org_data));
    return FALSE; // <value> was already copied by another thread.
  }
  // alive {K, V`}

  // Install the key in the new table.
  volatile lmn_key_t *org_key = (lmn_key_t*)&original->buckets[index];

  int rep_is_empty;
  lmn_word rep_index = cc_hashmap_lookup(replica, LMN_PTR_VAL(org_key), &rep_is_empty);
  if (rep_index == CC_PROB_FAIL) {
    fprintf(stderr, "prob fail at replica table {%d, %u, %p} {count:%d}\n", index, LMN_PTR_VAL(org_key), LMN_PTR_VAL(org_data), cc_hashmap_count(replica));
    //LMN_ASSERT(rep_index != CC_PROB_FAIL);
    if (replica->next == NULL) {
      cc_hashmap_start_copy(replica);
    }
    LMN_ASSERT(replica->next != NULL);
  }

  if (rep_is_empty) {
    if (!LMN_CAS(&replica->buckets[rep_index], CC_DOES_NOT_EXIST, LMN_PTR_VAL(org_key))) {
      LMN_DBG("retry copy entry {%u, %u, %p}\n", rep_index, LMN_PTR_VAL(org_key), LMN_PTR_VAL(org_data));
      return cc_hashmap_copy_entry(original, index, key_hash, replica); // retry recursive tail-call
    }
  }

  // LMN_PTR_VAL(org_data) = (lmn_data_t*)STRIP_TAG((lmn_word)LMN_PTR_VAL(org_data), TAG1); // remove tag

  int cas_succ = 0;
  if (LMN_PTR_VAL(org_data) != CC_COPIED_VALUE) {
    //LMN_DBG("insert replica {%u, %u} original {%u, %u, %u}\n", replica->buckets[rep_index], replica->data[rep_index], index, original->buckets[index], original->data[index]);
    cas_succ = LMN_CAS(&replica->data[rep_index], CC_DOES_NOT_EXIST, LMN_PTR_VAL(org_data));
  } else {
    // LMN_DBG("original {%u, %u, %p} replica {%u, %p}\n", rep_index, LMN_PTR_VAL(org_key), LMN_PTR_VAL(org_data), replica->buckets[rep_index], replica->data[rep_index]);
    return FALSE;
  }

  if (replica->data[rep_index] == CC_COPIED_VALUE) {
    LMN_DBG("original {%u, %u, %p} replica {%u, %p}\n", rep_index, LMN_PTR_VAL(org_key), LMN_PTR_VAL(org_data), replica->buckets[rep_index], replica->data[rep_index]);
    LMN_DBG("%p\n", CC_COPIED_VALUE);
    LMN_ASSERT(replica->data[rep_index] != CC_COPIED_VALUE);
  }

  LMN_PTR_VAL(org_data) = CC_COPIED_VALUE;

  if (cas_succ) {
    return TRUE;
  }

  LMN_DBG("%sanother thread completed the copy replica {%u, %u}%s\n", LMN_TERMINAL_RED, replica->buckets[rep_index], replica->data[rep_index], LMN_TERMINAL_DEFAULT);
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
    //if (!panic) {
    //  limit = cc_hashmap_tbl_size(map);//CC_ENTRIES_PER_COPY_CHUNK;
    //  // Reserve some entries for this thread to copy. There is a race condition here because the
    //  // fetch and add isn't atomic, but that is ok.
    //  map->copy_scan = x + limit;

    //  // <copy_scan> might be larger than the size of the table, if some thread stalls while
    //  // copying. In that case we just wrap around to the begining and make another pass through
    //  // the table.
    //  x = (x & map->bucket_mask);
    //} else {
    //  // scan the whole table
    //  int section = (cc_hashmap_tbl_size(map) / GetCurrentThreadCount());
    //  x = GetCurrentThreadId() * section;
    //  if (LMN_UNLIKELY(GetCurrentThreadCount() - 1 == GetCurrentThreadId())) {
    //    limit = cc_hashmap_tbl_size(map) - x;
    //  } else {
    //    limit = section;
    //  }
    //}
    int section = (cc_hashmap_tbl_size(map) / GetCurrentThreadCount());
    x = GetCurrentThreadId() * section;
    if (LMN_UNLIKELY(GetCurrentThreadCount() - 1 == GetCurrentThreadId())) {
      limit = cc_hashmap_tbl_size(map) - x;
    } else {
      limit = section;
    }
    //map->copy_scan = x + limit

    //printf("x:%d, tbl_size:%d\n", x, cc_hashmap_tbl_size(map));
    // Copy the entries
    for (int i = 0; i < limit; ++i) {
      num_copied += cc_hashmap_copy_entry(map, x + i, 0, map->next);
      LMN_ASSERT(x < cc_hashmap_tbl_size(map));
    }
    //LMN_DBG("tbl_size %d (x, last) : (%d, %d) thread:%d, num_copied:%d\n", cc_hashmap_tbl_size(map), x, x + limit, GetCurrentThreadId(), num_copied);
    if (num_copied != 0) {
      total_copied = LMN_ATOMIC_ADD(&map->num_entries_copied, num_copied);
      //total_copied = map->num_entries_copied = map->num_entries_copied + num_copied;
    }
  }

  // printf("%d\n", total_copied);
  return (total_copied == cc_hashmap_tbl_size(map));
}

// char ht_char[10000];

lmn_data_t cc_hashmap_put_inner(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  int is_empty;
  lmn_word index = cc_hashmap_lookup(map, key, &is_empty);
  if (key == 3905) {
  }

  if (LMN_UNLIKELY(index == CC_PROB_FAIL)) {
    LMN_DBG("[debug] cc_hashmap_put_inner: prob fail, {?, %u, %u} current:{tbl_size:%d, count:%d}, next:%p thread:%d\n", key, data, cc_hashmap_tbl_size(map), cc_hashmap_count(map), map->next, GetCurrentThreadId());
    if (map->next == NULL) {
      cc_hashmap_start_copy(map);
    }
    LMN_ASSERT(map->next);
    return CC_COPIED_VALUE;
  }

  if (is_empty) {
    if (!LMN_CAS(&map->buckets[index], CC_DOES_NOT_EXIST, key)) {
      // LMN_DBG("[debug] cc_hashmap_put_inner: retry put entry {%u, %u, %u} thread:%d\n", index, key, data, GetCurrentThreadId());
      return cc_hashmap_put_inner(map, key, data); // retry
    }
    map->data[index] = data;
    //int test = key;
    //if (test < 1000 && ht_char[test] == 0) {
    //  ht_char[test] = index;
    //} else if(test < 1000 && ht_char[test]) {
    //  stack_trace();
    //  LMN_DBG("[debug] cc_hashmap_put_inner: re put entry %u {%u, %u, %u} thread:%d %d\n", ht_char[test], index, key, data, GetCurrentThreadId(), cc_hashmap_tbl_size(map));
    //}
    //LMN_DBG("[debug] cc_hashmap_put_inner: put entry {%u, %u, %u} thread:%d\n", index, key, data, GetCurrentThreadId());
    //LMN_CAS(&(map->size), map->size, map->size + 1);
    cc_hashmap_inc_count(map);
  } else {
    // entry is immutable
    LMN_DBG("[debug] cc_hashmap_put_inner: Immutable value {%u, %u, %u} thread:%d\n", index, key, data, GetCurrentThreadId());
    return CC_IMMUTABLE_FAIL;
  }
  return data;
}

lmn_data_t cc_hashmap_find_inner(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word index = cc_hashmap_find_bucket(map, key);

  if (index == CC_PROB_FAIL) {
    LMN_LOG("[debug log] cc_hashmap_find_inner prob fail {??, %u, ??}\n", key);
    if (map->next != NULL) 
      return cc_hashmap_find_inner(map->next, key);
    return CC_DOES_NOT_EXIST;
  }
  volatile lmn_data_t* data = &map->data[index];
  if (LMN_UNLIKELY(LMN_PTR_VAL(data) == CC_COPIED_VALUE)) {
    lmn_data_t ret = cc_hashmap_find_inner(map->next, key);
    return ret;
  } else if (LMN_PTR_VAL(data) == CC_DOES_NOT_EXIST) {
    LMN_LOG("[debug log] cc_hashmap_find_inner: partially put {%u, %u, %u}\n", index, key, LMN_PTR_VAL(data));
    return cc_hashmap_find_inner(map, key);
  }
  LMN_LOG("[debug] cc_hashmap_find_inner: find entry {%u, %u, %u}\n", index, key, LMN_PTR_VAL(data));
  return LMN_PTR_VAL(data);
}

/*
 * public function
 */

void lmn_hashmap_init(lmn_hashmap_t *lmn_map) {
  lmn_map->current = lmn_malloc(cc_hashmap_t);
  cc_hashmap_init_inner(lmn_map->current, 1 << 20);
}

int lmn_hashmap_count(lmn_hashmap_t *lmn_map) {
  return cc_hashmap_count(lmn_map->current);
}

void cc_hashmap_free(cc_hashmap_t *map) {
  if (map == NULL) return;
  lmn_free((void*)map->buckets);
  lmn_free((void*)map->data);
  if (map->count != NULL)
    lmn_free((void*)map->count);
  cc_hashmap_free(map->next);
}

void lmn_hashmap_free(lmn_hashmap_t *lmn_map) {
  cc_hashmap_t *map = lmn_map->current;
  cc_hashmap_free(map);
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
        LMN_DBG("%s[debug] lmn_hashmap_put copy finished new_map_size:%d%s\n", LMN_TERMINAL_GREEN, cc_hashmap_tbl_size(lmn_map->current), LMN_TERMINAL_DEFAULT);
        map = map->next;
      }
    }
  }
  // put last table
  while (map->next != NULL) {
    int is_empty;
    int ret = cc_hashmap_lookup(map, key, &is_empty);
    if (is_empty == FALSE && ret != CC_PROB_FAIL) {
      LMN_DBG("[debug] is not empty {%u, %p}\n", key, data);
      return ;
    }
    map = map->next;
  }
  while (cc_hashmap_put_inner(map, key, data) == CC_COPIED_VALUE && map->next != NULL) {
    map = map->next;
  }
}

}
}
}

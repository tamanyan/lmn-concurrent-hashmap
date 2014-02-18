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
#define CC_PROB_FAIL -1
#define THRESHOLD 2

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
      "addr:%p tbl-size:%d\n", 
      map, 
      cc_hashmap_tbl_size(map)
  );
  if (content) {
    for (int i = 0; i < 10; i++) {
      LMN_DBG("[%d]:[%d]\n", map->buckets[i], map->data[i]);
    }
  }
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
      //LMN_DBG("[debug] cc_hashmap_put_inner: retry hash, key %u, thread:%d\n", key, GetCurrentThreadId());
    }
    offset = hash<lmn_word>(offset);
    count++;
  }
  LMN_PTR_VAL(is_empty) = FALSE;
  return CC_PROB_FAIL;
}


inline void cc_hashmap_init_inner(cc_hashmap_t *map, lmn_word scale) {
  map->buckets      = lmn_calloc(lmn_key_t,  scale);
  map->data         = lmn_calloc(lmn_data_t, scale);
  map->bucket_mask  = scale - 1;
  map->count        = lmn_calloc(int, 100);
}


inline lmn_data_t cc_hashmap_put_inner(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  int is_empty;
  lmn_word index = cc_hashmap_lookup(map, key, &is_empty);

  if (LMN_UNLIKELY(index == CC_PROB_FAIL)) {
    LMN_DBG("[debug] cc_hashmap_put_inner: prob fail, {?, %u, %u} current:{tbl_size:%d, count:%d}, thread:%d\n", key, data, cc_hashmap_tbl_size(map), cc_hashmap_count(map), GetCurrentThreadId());
    fprintf(stderr, "full!!!!\n");
    exit(1);
    return NULL;
  }

  if (is_empty) {
    if (!LMN_CAS(&map->buckets[index], CC_DOES_NOT_EXIST, key)) {
      LMN_DBG("[debug] cc_hashmap_put_inner: retry put entry {%u, %u, %u} thread:%d\n", index, key, data, GetCurrentThreadId());
      return cc_hashmap_put_inner(map, key, data); // retry
    }
    map->data[index] = data;
    //cc_hashmap_inc_count(map);
  } else {
    // entry is immutable
    //LMN_DBG("[debug] cc_hashmap_put_inner: Immutable value {%u, %u, %u} thread:%d\n", index, key, data, GetCurrentThreadId());
    return CC_IMMUTABLE_FAIL;
  }
  return data;
}

inline lmn_data_t cc_hashmap_find_inner(cc_hashmap_t *map, lmn_key_t key) {
  int is_empty;
  int ret = cc_hashmap_lookup(map, key, &is_empty);
  if (is_empty == TRUE && ret != CC_PROB_FAIL) {
    return CC_DOES_NOT_EXIST;
  } else if (is_empty == FALSE && ret != CC_PROB_FAIL) {
    return map->data[ret];
  } else if (ret == CC_PROB_FAIL) {
    fprintf(stderr, "find invalid key\n");
    exit(1);
  }
  fprintf(stderr, "find error\n");
  exit(1);
}

/*
 * public function
 */

void lmn_hashmap_init(lmn_hashmap_t *lmn_map) {
  lmn_map->current = lmn_malloc(cc_hashmap_t);
  cc_hashmap_init_inner(lmn_map->current, LMN_DEFAULT_SIZE);
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
  cc_hashmap_put_inner(map, key, data);
}

}
}
}

/**
 * @file   cc_hashmap.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "cc_hashmap.h"
#include <assert.h>

namespace lmntal {
namespace concurrent {
namespace hashmap {

#define CC_CACHE_LINE_SIZE_FOR_UNIT64 8
#define CC_PROB_FAIL -1
#define CC_INVALID_BUCKET -1
#define THRESHOLD 1

int call_count = 0;
int prob_count = 0;

/*
 * private functions
 */
inline lmn_word cc_hashmap_find_free_bucket(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word             offset = hash<lmn_word>(key);
  volatile lmn_key_t *buckets = map->buckets;
  lmn_word               mask = map->bucket_mask;
  int                   count = 0;
  if (key == CC_INVALID_BUCKET) {
    fprintf(stderr, "find invalid key\n");
    assert(key != CC_INVALID_BUCKET);
  }

  call_count++;
  while (count < THRESHOLD) {
    // Walk Cache line
    for (int i = 0; i < CC_CACHE_LINE_SIZE_FOR_UNIT64; i++) {
      prob_count++;
      if (buckets[(offset + i) & mask] == CC_INVALID_BUCKET)  {
        return (offset + i) & mask;
      }
    }
    offset = hash<lmn_word>(offset);
    count++;
  }
  return CC_PROB_FAIL;
}

inline lmn_word cc_hashmap_find_bucket(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word             offset = hash<lmn_word>(key);
  volatile lmn_key_t *buckets = map->buckets;
  lmn_word               mask = map->bucket_mask;
  int                   count = 0;
  if (key == CC_INVALID_BUCKET) {
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

/*
 * public function
 */
void cc_hashmap_init(cc_hashmap_t *map) {
  map->buckets       = lmn_calloc(lmn_key_t,  LMN_DEFAULT_SIZE);
  map->data          = lmn_calloc(lmn_data_t, LMN_DEFAULT_SIZE);
  map->bucket_mask  = LMN_DEFAULT_SIZE - 1;
  map->size         = 0;
  memset((void*)map->buckets, CC_INVALID_BUCKET, sizeof(lmn_key_t)*LMN_DEFAULT_SIZE);
}

void cc_hashmap_free(cc_hashmap_t *map) {
  printf("call_count:%d\n", call_count);
  printf("prob_count:%d\n", prob_count);
  printf("%lf prob/call\n", (float)prob_count/call_count);
}

lmn_data_t cc_hashmap_find(cc_hashmap_t *map, lmn_key_t key) {
  lmn_word index = cc_hashmap_find_bucket(map, key);
  if (index != CC_PROB_FAIL) {
    return map->data[index];
  } else {
    return LMN_HASH_EMPTY_DATA;
  }
}

void cc_hashmap_put(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  lmn_word index = cc_hashmap_find_free_bucket(map, key);

  if (index != CC_PROB_FAIL && map->buckets[index] == CC_INVALID_BUCKET) {
    if (!LMN_CAS(&map->buckets[index], CC_INVALID_BUCKET, data)) {
      printf("insert fail retry key:%d, index:%d\n", key, index);
      cc_hashmap_put(map, key, data);
      return ;
    } else {
      //printf("insert key:%d, index:%d\n", key, index);
      map->data[index] = data;
      LMN_ATOMIC_ADD(&(map->size), 1);
    }
  } else {
    //printf("insert fail key:%d, index:%d\n", key, index);
  }
}

}
}
}

/**
 * @file   hashmap.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef HASHMAP_H
#  define HASHMAP_H

#include <semaphore.h>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
using namespace std;

#define LMN_CAS(a_ptr, a_old, a_new) __sync_bool_compare_and_swap(a_ptr, a_old, a_new)
#define LMN_ATOMIC_ADD(ptr, v)       __sync_fetch_and_add(ptr, v)
#define LMN_ATOMIC_SUB(ptr, v)       __sync_fetch_and_sub(ptr, v)
#define LMN_ATOMIC_AND_OR(ptr, v)    __sync_fetch_and_or(ptr, v)

#define LMN_LIKELY(x)    __builtin_expect(!!(x), 1)
#define LMN_UNLIKELY(x)  __builtin_expect(!!(x), 0)

#define LMN_PREFETCH(addr, rw, locality) __builtin_prefetch(addr, rw, locality)

#define LMN_PTR_VAL(ptr) (*ptr)

// #define LMN_DEBUG

#define LMN_TERMINAL_RED "\x1b[31m"
#define LMN_TERMINAL_GREEN "\x1b[32m"
#define LMN_TERMINAL_YELLOW "\x1b[33m"
#define LMN_TERMINAL_DEFAULT "\x1b[39m"

#ifdef LMN_DEBUG
#define LMN_DBG_V(...) printf(__VA_ARGS__)
#define LMN_DBG(...) printf(__VA_ARGS__)
#define LMN_SWITCH_COLOR(color) LMN_DBG(color)
#define LMN_ASSERT(expr) assert(expr);
#else
#define LMN_ASSERT(expr) (NULL)
#define LMN_DBG_V(...) (NULL)
#define LMN_DBG(...) (NULL) 
#define LMN_SWITCH_COLOR (NULL)
#endif

//#define LMN_LOG

#ifdef LMN_LOG
#define LMN_LOG(...) LMN_DBG(__VA_ARGS__)
#else
#define LMN_LOG(...) (NULL)
#endif

#define FALSE 0
#define TRUE 1

#define LMN_HASH_EMPTY      0
#define LMN_HASH_BUSY       1
#define LMN_HASH_EMPTY_DATA 0
#define LMN_HASH_EMPTY_KEY  0

//#define LMN_DEFAULT_SIZE    16777216
#define LMN_DEFAULT_SIZE (1 << 20)
//#define LMN_DEFAULT_SIZE    1024 * 32

typedef unsigned long lmn_word;
typedef lmn_word lmn_key_t;
typedef void*    lmn_data_t;
typedef void*    lmn_map_t;

namespace lmntal {
namespace concurrent {
namespace hashmap {

#define HASHMAP_SEGMENT 12

#define lmn_malloc(type)       (type*)malloc(sizeof(type))
#define lmn_calloc(type, size)       (type*)calloc((size), sizeof(type))
#define lmn_free(ptr)                free(ptr);

template <typename T>
inline lmn_word hash(T val) {
  lmn_word key = static_cast<lmn_word>(val);
  const unsigned int m = 0x5bd1e995;
  const int r = 24; 

  // Initialize the hash to a 'random' value
  unsigned int h = 8;

  unsigned int k1 = (unsigned int)(key >> 32);
  unsigned int k2 = (unsigned int)key;

  k1 *= m;  
  k1 ^= k1 >> r;  
  k1 *= m;  

  k2 *= m;  
  k2 ^= k2 >> r;  
  k2 *= m;  

  // Mix 4 bytes at a time into the hash

  h *= m;  
  h ^= k1; 
  h *= m;  
  h ^= k2; 

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.

  h ^= h >> 13; 
  h *= m;
  h ^= h >> 15; 

  return h;
}

typedef enum {
  LMN_CLOSED_ADDRESSING = 0,
  LMN_LOCK_FREE_CLOSED_ADDRESSING,
  LMN_MC_CLIFF_CLICK
} hashmap_type_t;

typedef lmn_data_t  (*hashmap_find_t)(lmn_map_t, lmn_word);
typedef void        (*hashmap_put_t)(lmn_map_t, lmn_word, lmn_data_t);
typedef void        (*hashmap_init_t)(lmn_map_t);
typedef void        (*hashmap_free_t)(lmn_map_t);

typedef struct _hashmap_impl_t {
  hashmap_find_t find;
  hashmap_put_t put;
  hashmap_init_t init;
  hashmap_free_t free;
} hashmap_impl_t;

typedef struct _hashmap_t {
  lmn_map_t          data;
  hashmap_impl_t impl;
} hashmap_t;

void hashmap_init(hashmap_t *map, hashmap_type_t type);

inline lmn_data_t hashmap_find(hashmap_t *map, lmn_key_t key) {
  return map->impl.find(map->data, key);
}

inline void hashmap_put(hashmap_t *map, lmn_key_t key, lmn_data_t data) {
  map->impl.put(map->data, key, data);
}

inline void hashmap_free(hashmap_t *map) {
  map->impl.free(map->data);
}

}
}
}

#endif /* ifndef HASHMAP_H */


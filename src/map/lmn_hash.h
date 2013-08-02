/**
 * @file   lmn_hash.h
 * @brief  hash
 * @author Taketo Yoshida
 */
#ifndef LMN_HASH_H
#  define LMN_HASH_H

#include "lmn_map_util.h"

typedef lmn_word lmn_hash_t;
#define LMN_HASH_EMPTY 0
#define LMN_HASH_BUSY  1

inline lmn_hash_t lmn_hash_calc(lmn_key_t key) {
#ifdef __x86_64__
  key ^= (key << 30) ^ 0xcd7dcd7dcd7dcd7d;
  key ^= (key >> 20);
  key ^= (key <<  6);
  key ^= (key >> 12);
  key ^= (key <<  4) + (key << 28);
  key ^= (key >> 31);
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

#endif /* ifndef LMN_HASH_H */


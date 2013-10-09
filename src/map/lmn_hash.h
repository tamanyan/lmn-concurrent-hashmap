/**
 * @file   lmn_hash.h
 * @brief  hash
 * @author Taketo Yoshida
 */
#ifndef LMN_HASH_H
#  define LMN_HASH_H

#include "lmn_map_util.h"

typedef u32 lmn_hash_t;
#define LMN_HASH_EMPTY      0
#define LMN_HASH_BUSY       1
#define LMN_HASH_EMPTY_DATA 0
#define LMN_HASH_EMPTY_KEY  0

#ifndef __linux__

inline lmn_hash_t lmn_hash_calc(lmn_key_t key) {
#ifdef __x86_64__
  /* Robert Jenkins' 32 bit Mix Function */
  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);

  /* Knuth's Multiplicative Method */
  key = (key >> 3) * 2654435761;
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

#endif

#endif /* ifndef LMN_HASH_H */


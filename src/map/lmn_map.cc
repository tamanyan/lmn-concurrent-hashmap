/**
 * @file   lmn_map.cc
 * @brief  abstract map structure
 * @author Taketo Yoshida
*/
#include "lmn_map.h"

#ifdef __linux__

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


void lmn_map_init(lmn_map_t* map, const lmn_map_type_t type) {
  switch (type) {
    case LMM_MAP_HASH_OPEN_ADDRESSING:
    case LMM_MAP_HASH_CLOSED_ADDRESSING:
      break;
    default:
      return;
  }
}

void lmn_map_put(lmn_map_t *map, lmn_key_t key, lmn_data_t data) {

}

/**
 * @file   hash.hpp
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef HASH_H
#  define HASH_H

#include "hashmap.hpp"

namespace lmntal {
namespace concurrent {
namespace hashmap {

template <typename T>
lmn_word hash(T val) {
  lmn_word key = static_cast<lmn_word>(val);
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

}
}
}

#endif /* ifndef HASH_H */

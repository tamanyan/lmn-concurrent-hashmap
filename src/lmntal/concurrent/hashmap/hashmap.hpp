/**
 * @file   hashmap.hpp
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef HASHMAP_H
#  define HASHMAP_H

#include "hash.hpp"
#include <semaphore.h>
#include <iostream>
using namespace std;

#define LMN_CAS(a_ptr, a_old, a_new) __sync_bool_compare_and_swap(a_ptr, a_old, a_new)
#define LMN_ATOMIC_ADD(ptr, v)       __sync_fetch_and_add(ptr, v)
#define LMN_ATOMIC_SUB(ptr, v)       __sync_fetch_and_sub(ptr, v)

#define LMN_HASH_EMPTY      0
#define LMN_HASH_BUSY       1
#define LMN_HASH_EMPTY_DATA 0
#define LMN_HASH_EMPTY_KEY  0

//#define LMN_DEFAULT_SIZE    16777216
#define LMN_DEFAULT_SIZE    1024

typedef unsigned long       lmn_word;

namespace lmntal {
namespace concurrent {
namespace hashmap {

template<typename Key, typename Value>
class HashMap {
public:
  HashMap() {}
  virtual ~HashMap() {}

  virtual Value& Get(const Key& key) = 0;
  virtual void Put(const Key& key, const Value& value) = 0;
  virtual bool Exist(const Key& key) = 0;
  virtual void Extend() = 0;
  virtual uint64_t Size() = 0;
  virtual int Count() = 0;

protected:
};

}
}
}

#endif /* ifndef HASHMAP_H */


/**
 * @file   concurrent_chain_hashmap.hpp
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef CONCURRENT_CHAIN_HASHMAP_H
#  define CONCURRENT_CHAIN_HASHMAP_H

#include "chain_hashmap.hpp"
#include "../thread.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

/*
 * definition ConcurrentChainHashMap
 */
template<typename Key, typename Value>
class ConcurrentChainHashMap : public ChainHashMap<Key, Value> {
public:
  ConcurrentChainHashMap() : ChainHashMap<Key, Value>() {
    {
      pthread_mutexattr_t mattr[SECTION_SIZE];
      for (int i = 0; i < SECTION_SIZE; i++) {
        pthread_mutexattr_init(&mattr[i]);
        pthread_mutexattr_setpshared(&mattr[i], PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mutexs[i], &mattr[i]);
      }
    }
  }

  ~ConcurrentChainHashMap() {
  }

  Value& Get(const Key& key) {
    lmn_word bucket = hash<Key>(key) & this->bucket_mask;
    Value value;
    {
      pthread_mutex_lock(&mutexs[bucket % SECTION_SIZE]);
      value = ChainHashMap<Key, Value>::Get(key);
      pthread_mutex_unlock(&mutexs[bucket % SECTION_SIZE]);
    }
    return value;
  }

  void Put(const Key& key, const Value& value) {
    lmn_word hash_value = hash<Key>(key);
    lmn_word bucket = hash_value & this->bucket_mask;
    {
      pthread_mutex_lock(&mutexs[bucket % SECTION_SIZE]);
      lmn_word new_bucket = hash_value & this->bucket_mask;
      if (new_bucket != bucket) {
        pthread_mutex_unlock(&mutexs[bucket % SECTION_SIZE]);
        pthread_mutex_lock(&mutexs[new_bucket % SECTION_SIZE]);
        bucket = new_bucket;
      }
      this->PutEntry(bucket, key, value);
      pthread_mutex_unlock(&mutexs[bucket % SECTION_SIZE]);
    }
    if (this->size > this->bucket_mask * 0.75) {
      Extend();
    }
  }

  void Extend() {
    //printf("extend start %d\n", GetCurrentThreadId());
    int i, j;
    Entry<Key, Value> **assumed_table = this->tbl;
    for (i = 0; i < SECTION_SIZE; i++) {
      if (assumed_table != this->tbl)
        break;
      pthread_mutex_lock(&mutexs[i]);
    }
    if (this->size > this->bucket_mask * 0.75 && i == SECTION_SIZE) {
      //printf("enter extend %d\n", GetCurrentThreadId());
      ChainHashMap<Key, Value>::Extend();
    }
    for (j = 0; j < i; j++) {
      pthread_mutex_unlock(&mutexs[j]);
    }
    //printf("extend end %d\n", GetCurrentThreadId());
  }

protected:
  pthread_mutex_t   mutexs[SECTION_SIZE];
};

}
}
}
#endif /* ifndef CONCURRENT_CHAIN_HASHMAP_H */


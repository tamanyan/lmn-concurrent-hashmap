/**
 * @file   chain_hashmap.hpp
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef CHAIN_HASHMAP_H
#  define CHAIN_HASHMAP_H

#include <iostream>
#include "hashmap.hpp"
using namespace std;

namespace lmntal {
namespace hashmap {

#define SECTION_SIZE 12

/*
 * definition ChainHashMap
 */
template<typename Key, typename Value>
class ChainHashMap : public HashMap<Key, Value> {
public:
  typedef struct _Entry {
    Key   key;
    Value value;
    struct _Entry* entry;
  } Entry;

  ChainHashMap() : size(0), bucket_mask(LMN_DEFAULT_SIZE - 1) {
    tbl = new Entry*[LMN_DEFAULT_SIZE];
    {
      pthread_mutexattr_t mattr[SECTION_SIZE];
      int i;
      for (i = 0; i < SECTION_SIZE; i++) {
        pthread_mutexattr_init(&mattr[i]);
        pthread_mutexattr_setpshared(&mattr[i], PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mutexs[i], &mattr[i]);
      }
    }
  }

  ~ChainHashMap() {
  
  }

  Value& Get(const Key& key) {
    cout << "test" << endl;
  }

  void Put(const Key& key, const Value& value) {
  
  }

  bool Exist(const Key& key) {
    return false;
  }

  void Extend() {
  
  }

private:
  Entry**           volatile tbl;
  lmn_word          volatile bucket_mask;
  lmn_word          volatile size;
  pthread_mutex_t   mutexs[SECTION_SIZE];
};

}
}

#endif /* ifndef CHAIN_HASHMAP_H */


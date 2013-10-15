/**
 * @file   lock_free_chain_hashmap.hpp
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef LOCK_FREE_CHAIN_HASHMAP_H
#  define LOCK_FREE_CHAIN_HASHMAP_H

namespace lmntal {
namespace concurrent {
namespace hashmap {

#include "chain_hashmap.hpp"
#include "../thread.h"

/*
 * definition ConcurrentChainHashMap
 */
template<typename Key, typename Value>
class LockFreeChainHashMap : public ChainHashMap<Key, Value> {
public:
  LockFreeChainHashMap(int num_thread) : num_thread(num_thread), during_extend(0), ChainHashMap<Key, Value>() {
    mutexs = new pthread_mutex_t[num_thread];
    pthread_mutexattr_t mattr[SECTION_SIZE];
    for (int i = 0; i < SECTION_SIZE; i++) {
      pthread_mutexattr_init(&mattr[i]);
      pthread_mutexattr_setpshared(&mattr[i], PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&mutexs[i], &mattr[i]);
    }
  }

  ~LockFreeChainHashMap() {
    delete[] mutexs;
  }

  Value& Get(const Key& key) {
    if (during_extend) {
      pthread_mutex_lock(&mutexs[GetCurrentThreadId() % num_thread]);
      pthread_mutex_unlock(&mutexs[GetCurrentThreadId() % num_thread]);
    }

    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    Entry<Key, Value> *ent = this->FindEntry(bucket, key);

    return ent != NULL ? ent->value : this->empty_value;
  }

  void Put(const Key& key, const Value& value) {
    if (during_extend) {
      pthread_mutex_lock(&mutexs[GetCurrentThreadId() % num_thread]);
      pthread_mutex_unlock(&mutexs[GetCurrentThreadId() % num_thread]);
    }

    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    this->PutEntry(bucket, key, value);

    if (this->size > this->bucket_mask * 0.75) {
      Extend();
    }
  }

  void Extend() {
    for (int i = 0; i < num_thread; i++) {
      pthread_mutex_lock(&mutexs[i]);
    }
    //printf("enter cs id: %d\n", GetCurrentThreadId() % num_thread);
    int count = 0;
    if (this->size > this->bucket_mask * 0.75) {
      LMN_ATOMIC_ADD(&(during_extend), 1);
      ChainHashMap<Key, Value>::Extend();
      LMN_ATOMIC_SUB(&(during_extend), 1);
    }
    for (int i = 0; i < num_thread; i++) {
      pthread_mutex_unlock(&mutexs[i]);
    }
  }

protected:
  pthread_mutex_t*   mutexs;
  pthread_mutex_t   wait;
  int               during_extend;
  int               num_thread;

  Entry<Key, Value>* FindEntry(lmn_word bucket, Key key) {
    Entry<Key, Value> *ent      = this->tbl[bucket];
    int count = 0;

    while(ent != LMN_HASH_EMPTY) {
      if (ent->key == key) {
        return ent;
      }
      ent = ent->next;
      count++;
    }
    //printf("error entry key:%d count:%d\n", key, count);
    return NULL;
  }

  void PutEntry(lmn_word bucket, Key key, Value value) {
    Entry<Key, Value>    **ent    = &this->tbl[bucket];
    Entry<Key, Value>     *cur, *tmp;

    if ((*ent) == LMN_HASH_EMPTY) {
      Entry<Key, Value> *tmp, *new_ent = NULL;
      do {
        tmp = *ent;
        if (new_ent == NULL) {
          new_ent = new Entry<Key, Value>();
          new_ent->next = NULL;
        } else {
          new_ent->next = (*ent);
          //dbgprint("retry insert key:%d, data:%d\n", key, value);
        }
      } while(!LMN_CAS(&(*ent), tmp, new_ent));
    } else {
      Entry<Key, Value> *cur, *tmp, *new_ent = NULL;
      do {
        cur = *ent;
        tmp = *ent;
        do {
          if (cur->key == key) {
            cur->value = value;
            if (new_ent == NULL) {
              delete new_ent;
              return;
            }
          }
        } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
        if (new_ent == NULL) {
          new_ent = new Entry<Key, Value>();
        } else {
          //dbgprint("retry insert key:%d, data:%d\n", key, value);
        }
        new_ent->next = (*ent);
        // CAS target, old, new
      } while(!LMN_CAS(&(*ent), tmp, new_ent));
      //dbgprint("insert key:%d, data:%d\n", key, value);
    }
    (*ent)->key  = key;
    (*ent)->value = value;
    LMN_ATOMIC_ADD(&(this->size), 1);
    if (!FindEntry(bucket, key)) {
      //printf("error2 %d\n", key);
    }
  }
};

}
}
}

#endif /* ifndef LOCK_FREE_CHAIN_HASHMAP_H */


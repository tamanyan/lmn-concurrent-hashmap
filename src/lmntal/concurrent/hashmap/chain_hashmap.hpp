/**
 * @file   chain_hashmap.hpp
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef CHAIN_HASHMAP_H
#  define CHAIN_HASHMAP_H

#include <iostream>
#include <functional>
#include "hashmap.hpp"
using namespace std;

namespace lmntal {
namespace concurrent {
namespace hashmap {

#define SECTION_SIZE 12

#define chain_foreach(map, ent, code)             \
  {                                                          \
    int __i;                                                 \
    for (__i = 0; __i < ((map)->bucket_mask + 1); __i++) {    \
      (ent) = (map)->tbl[__i];                                \
      while ((ent) != LMN_HASH_EMPTY) {                      \
        (code);                                              \
        (ent) = (ent)->next;                                 \
      }                                                      \
    }                                                        \
  }

template<typename Key, typename Value>
struct Entry {
  Key   key;
  Value value;
  struct Entry<Key, Value> *next;
};

/*
 * definition ChainHashMap
 */
template<typename Key, typename Value>
class ChainHashMap : public HashMap<Key, Value> {
public:
  ChainHashMap() : size(0), bucket_mask(LMN_DEFAULT_SIZE - 1) {
    tbl = new Entry<Key, Value>*[LMN_DEFAULT_SIZE];
    empty_value = static_cast<Value>(NULL);
  }

  virtual ~ChainHashMap() {
    Entry<Key, Value> *ent;
    chain_foreach(this, ent, ({
        delete ent;
    }));
    delete tbl;
  }

  Value& Get(const Key& key) {
    lmn_word bucket = hash<Key>(key) & bucket_mask;
    Entry<Key, Value>* ent = FindEntry(bucket, key);
    return ent != NULL ? ent->value : empty_value;
  }

  void Put(const Key& key, const Value& value) {
    lmn_word   bucket = hash<Key>(key) & bucket_mask;
    PutEntry(bucket, key, value);
    // map->size++;
    if (size > bucket_mask * 0.75) {
      Extend();
    }
  }

  bool Exist(const Key& key) {
    return &Get(key) != &empty_value;
  }

  void Extend() {
    int i;
    lmn_word     new_size = bucket_mask + 1;
    lmn_word     old_size = new_size;
    Entry<Key, Value>      **new_tbl = new Entry<Key, Value>*[new_size <<= 1];
    Entry<Key, Value>      **old_tbl = tbl;
    Entry<Key, Value>       *ent, *next;
    lmn_word     bucket;
    lmn_word     new_bucket_mask = new_size - 1;

    dbgprint("rehash start old_size:%d new_size:%d\n", old_size, new_size);
    // lmn_chained_print_map(map);
    for (i = 0; i < old_size; i++) {
      ent = old_tbl[i];
      while (ent) {
        next = ent->next;
        bucket = (hash<Key>(ent->key) & new_bucket_mask);
        ent->next = new_tbl[bucket];
        new_tbl[bucket] = ent;
        ent = next;
      }
    }
    bucket_mask = new_bucket_mask;
    tbl         = new_tbl;
    //lmn_chained_print_map(map);
    delete old_tbl;
  }

  uint64_t Size() { return size; }

protected:
  struct Entry<Key, Value>**           volatile tbl;
  lmn_word          volatile bucket_mask;
  lmn_word          volatile size;
  Value             empty_value;

  Entry<Key, Value>* FindEntry(lmn_word bucket, Key key) {
    Entry<Key, Value> *ent      = tbl[bucket];

    while(ent != LMN_HASH_EMPTY) {
      if (ent->key == key) {
        return ent;
      }
      ent = ent->next;
    }
    return NULL;
  }

  void PutEntry(lmn_word bucket, Key key, Value value) {
    Entry<Key, Value>    **ent    = &tbl[bucket];
    Entry<Key, Value>     *cur, *tmp;

    if ((*ent) == LMN_HASH_EMPTY) {
      (*ent) = new Entry<Key, Value>();
      (*ent)->next = NULL;
    } else {
      cur = *ent;
      do {
        if (cur->key == key) {
          cur->value = value;
          return;
        }
      } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
      cur = new Entry<Key, Value>();
      cur->next = (*ent);
      (*ent) = cur;
      //dbgprint("insert key:%d, data:%d\n", key, value);
    }
    (*ent)->key  = key;
    (*ent)->value = value;
    LMN_ATOMIC_ADD(&(size), 1);
    
  }
};

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
    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    pthread_mutex_lock(&mutexs[bucket % SECTION_SIZE]);
    Entry<Key, Value> *ent = this->FindEntry(bucket, key);
    pthread_mutex_unlock(&mutexs[bucket % SECTION_SIZE]);
    return ent != NULL ? ent->value : this->empty_value;
  }

  void Put(const Key& key, const Value& value) {
    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    pthread_mutex_lock(&mutexs[bucket % SECTION_SIZE]);
    this->PutEntry(bucket, key, value);
    pthread_mutex_unlock(&mutexs[bucket % SECTION_SIZE]);
    if (this->size > this->bucket_mask * 0.75) {
      Extend();
    }
  }

  void Extend() {
    for (int i = 0; i < SECTION_SIZE; i++) {
      pthread_mutex_lock(&mutexs[i]);
    }
    ChainHashMap<Key, Value>::Extend();
    for (int i = 0; i < SECTION_SIZE; i++) {
      pthread_mutex_unlock(&mutexs[i]);
    }
  }

protected:
  pthread_mutex_t   mutexs[SECTION_SIZE];
};

/*
 * definition ConcurrentChainHashMap
 */
template<typename Key, typename Value>
class LockFreeChainHashMap : public ChainHashMap<Key, Value> {
public:
  LockFreeChainHashMap(int num_thread) : in_thread(0), num_thread(num_thread), during_extend(0), ChainHashMap<Key, Value>() {
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mattr);
  }

  ~LockFreeChainHashMap() {
  }

  Value& Get(const Key& key) {
    if (during_extend) {
      pthread_mutex_lock(&mutex);
    }
    LMN_ATOMIC_ADD(&(in_thread), 1);

    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    Entry<Key, Value> *ent = this->FindEntry(bucket, key);

    LMN_ATOMIC_SUB(&(in_thread), 1);
    return ent != NULL ? ent->value : this->empty_value;
  }

  void Put(const Key& key, const Value& value) {
    if (during_extend)
      pthread_mutex_lock(&mutex);
    LMN_ATOMIC_ADD(&(in_thread), 1);

    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    this->PutEntry(bucket, key, value);

    LMN_ATOMIC_SUB(&(in_thread), 1);
    
    if (this->size > this->bucket_mask * 0.75) {
      Extend();
    }
  }

  void Extend() {
    //cout << "Extend" << in_thread << endl;
    pthread_mutex_lock(&mutex);
    if (this->size > this->bucket_mask * 0.75) {
      LMN_ATOMIC_ADD(&(during_extend), 1);
      while(in_thread > 0) {}
      ChainHashMap<Key, Value>::Extend();
      LMN_ATOMIC_SUB(&(during_extend), 1);
    }
    pthread_mutex_unlock(&mutex);
  }

protected:
  pthread_mutex_t   mutex;
  pthread_mutex_t   wait;
  int               during_extend;
  int               num_thread;
  int               in_thread;

  Entry<Key, Value>* FindEntry(lmn_word bucket, Key key) {
    Entry<Key, Value> *ent      = this->tbl[bucket];

    while(ent != LMN_HASH_EMPTY) {
      if (ent->key == key) {
        return ent;
      }
      ent = ent->next;
    }
    return NULL;
  }

  void PutEntry(lmn_word bucket, Key key, Value value) {
    Entry<Key, Value>    **ent    = &this->tbl[bucket];
    Entry<Key, Value>     *cur, *tmp;

    if ((*ent) == LMN_HASH_EMPTY) {
      (*ent) = new Entry<Key, Value>();
      (*ent)->next = NULL;
    } else {
      Entry<Key, Value> *cur, *tmp, *new_ent = NULL;
      cur = *ent;
      do {
        tmp = *ent;
        do {
          if (cur->key == key) {
            cur->value = value;
            if (new_ent == NULL) lmn_free(new_ent)
              return;
          }
          //printf("%p ", cur->next);
        } while(cur->next != LMN_HASH_EMPTY && (cur = cur->next));
        if (new_ent == NULL)
          new_ent = new Entry<Key, Value>();
        new_ent->next = (*ent);
      } while(!LMN_CAS(&(*ent), tmp, new_ent));
      //dbgprint("insert key:%d, data:%d\n", key, value);
    }
    (*ent)->key  = key;
    (*ent)->value = value;
    LMN_ATOMIC_ADD(&(this->size), 1);
  }
};


}
}
}

#endif /* ifndef CHAIN_HASHMAP_H */


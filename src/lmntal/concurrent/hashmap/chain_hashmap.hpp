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
#include "../thread.h"
using namespace std;

namespace lmntal {
namespace concurrent {
namespace hashmap {

#define SECTION_SIZE 12

#define chain_foreach(map, type, ent, code)             \
  {                                                          \
    int __i;                                                 \
    for (__i = 0; __i < ((map)->bucket_mask + 1); __i++) {    \
      (ent) = (map)->tbl[__i];                                \
      while ((ent) != LMN_HASH_EMPTY) {                      \
        type __next = (ent)->next;                           \
        (code);                                              \
        (ent) = __next;                                 \
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
    Entry<Key, Value> *ent, *next;
    for (int i = 0; i < bucket_mask + 1; i++) {
      ent = tbl[i];
      while(ent != LMN_HASH_EMPTY) {
        next = ent->next;
        delete ent;
        ent = next;
      }
    }
    delete[] tbl;
  }

  Value& Get(const Key& key) {
    lmn_word bucket = hash<Key>(key) & bucket_mask;
    Entry<Key, Value>* ent = FindEntry(bucket, key);
    return ent != NULL ? ent->value : empty_value;
  }

  void Put(const Key& key, const Value& value) {
    lmn_word   bucket = hash<Key>(key) & bucket_mask;
    PutEntry(bucket, key, value);
    if (size > bucket_mask * 0.75) {
      Extend();
    }
  }

  bool Exist(const Key& key) {
    return &Get(key) != &empty_value;
  }

  void Dump() {
    int i;
    int count = 0;
    Entry<Key, Value> *ent;
    dbgprint("======================================================================\n", NULL);
    for (i = 0; i < (bucket_mask + 1); i++) {
      ent = tbl[i];
      dbgprint("(%d) ",i);
      while (ent != LMN_HASH_EMPTY) {
        dbgprint("%d:%d ", ent->key, ent->value);
        count++;
        ent = ent->next;
      }
      dbgprint("\n", NULL);
    }
    dbgprint("tbl size %d, count %d org_count %d\n", bucket_mask+1, count, size);
    dbgprint("======================================================================\n", NULL);
  }

  int Count() {
    int i;
    int count = 0;
    Entry<Key, Value> *ent;
    for (i = 0; i < (bucket_mask + 1); i++) {
      ent = tbl[i];
      while (ent != LMN_HASH_EMPTY) {
        count++;
        ent = ent->next;
      }
    }
    return count;
  }

  void Extend() {
    lmn_word     new_size = bucket_mask + 1;
    lmn_word     old_size = new_size;
    Entry<Key, Value>      **new_tbl = new Entry<Key, Value>*[new_size <<= 1];
    Entry<Key, Value>      **old_tbl = tbl;
    Entry<Key, Value>       *ent, *next;
    lmn_word     bucket;
    lmn_word     new_bucket_mask = new_size - 1;

    //dbgprint("extend old_size:%d, new_size:%d\n", old_size, new_size);
    for (int i = 0; i < old_size; i++) {
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
    delete old_tbl;
  }

  long Size() { return size; }

protected:
  struct Entry<Key, Value>**           volatile tbl;
  lmn_word          volatile bucket_mask;
  lmn_word          volatile size;
  Value             empty_value;

  Entry<Key, Value>* FindEntry(lmn_word bucket, Key key) {
    Entry<Key, Value> *ent      = tbl[bucket];

    while (ent != LMN_HASH_EMPTY) {
      if (ent->key == key) {
        return ent;
      }
      ent = ent->next;
    }
    //dbgprint("not found (%d,%d)\n", bucket, key);
    //cout << Count() << endl;
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

/*
 * definition ConcurrentChainHashMap
 */
template<typename Key, typename Value>
class LockFreeChainHashMap : public ChainHashMap<Key, Value> {
public:
  LockFreeChainHashMap(int num_thread) : in_thread(0), num_thread(num_thread), during_extend(0), ChainHashMap<Key, Value>() {
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
    LMN_ATOMIC_ADD(&(in_thread), 1);

    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    Entry<Key, Value> *ent = this->FindEntry(bucket, key);

    LMN_ATOMIC_SUB(&(in_thread), 1);
    return ent != NULL ? ent->value : this->empty_value;
  }

  void Put(const Key& key, const Value& value) {
    if (during_extend) {
      pthread_mutex_lock(&mutexs[GetCurrentThreadId() % num_thread]);
      pthread_mutex_unlock(&mutexs[GetCurrentThreadId() % num_thread]);
    }
    LMN_ATOMIC_ADD(&(in_thread), 1);

    lmn_word   bucket = hash<Key>(key) & this->bucket_mask;
    this->PutEntry(bucket, key, value);

    LMN_ATOMIC_SUB(&(in_thread), 1);
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
      //printf("enter Extend in_thread: %d\n", in_thread);
      ChainHashMap<Key, Value>::Extend();
      //printf("end Extend in_thread: %d\n", in_thread);
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
  int               in_thread;

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

#endif /* ifndef CHAIN_HASHMAP_H */


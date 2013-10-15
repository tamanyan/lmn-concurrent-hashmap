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

}
}
}

#endif /* ifndef CHAIN_HASHMAP_H */


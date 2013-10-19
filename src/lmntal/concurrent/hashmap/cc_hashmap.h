/**
 * @file   cc_hashmap.h
 * @brief
 * The hash table is a combination of the Shared Hash Tables for LTSmin and Cliff Click HashTable.
 * Cliff Click HashTable : http://www.stanford.edu/class/ee380/Abstracts/070221_LockFreeHash.pdf
 * Shared Hash Tables for LTSmin : http://fmcad10.iaik.tugraz.at/Papers/papers/12Session11/033Laarman.pdf
 * @author Taketo Yoshida
 */
#ifndef CC_HASHMAP_H
#  define CC_HASHMAP_H

#include "hashmap.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

typedef struct _cc_hashmap_t {
  lmn_key_t   volatile *buckets; // key index array
  lmn_data_t  volatile *data; // data index array
  lmn_word    volatile bucket_mask;
  int         volatile *count;
  int         volatile copy;
  lmn_word    copy_scan;
  lmn_word    num_entries_copied;
  struct _cc_hashmap_t* volatile next;
} cc_hashmap_t;

typedef struct _lmn_hashmap_t {
  cc_hashmap_t* current;
} lmn_hashmap_t;

void lmn_hashmap_init(lmn_hashmap_t *map);
lmn_data_t lmn_hashmap_find(lmn_hashmap_t *map, lmn_key_t key);
void lmn_hashmap_put(lmn_hashmap_t *map, lmn_key_t key, lmn_data_t data);
void lmn_hashmap_free(lmn_hashmap_t *map);
int lmn_hashmap_count(lmn_hashmap_t *map);

}
}
}

#endif /* ifndef CC_HASHMAP_H */


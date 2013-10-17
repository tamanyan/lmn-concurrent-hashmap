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
  lmn_word    volatile size;
  struct _cc_hashmap_t* volatile next;
} cc_hashmap_t;

void cc_hashmap_init(cc_hashmap_t *map);
lmn_data_t cc_hashmap_find(cc_hashmap_t *map, lmn_key_t key);
void cc_hashmap_put(cc_hashmap_t *map, lmn_key_t key, lmn_data_t data);
void cc_hashmap_free(cc_hashmap_t *map);

}
}
}

#endif /* ifndef CC_HASHMAP_H */


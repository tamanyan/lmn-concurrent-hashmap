/**
 * @file   hashmap.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "hashmap.h"
#include "chain_hashmap.h"
#include "lf_chain_hashmap.h"
#include "cc_hashmap.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

static const hashmap_impl_t CHAIN_HASHMAP_IMPL_HT = { 
  (hashmap_find_t)chain_find,
  (hashmap_put_t)chain_put,
  (hashmap_init_t)chain_init,
  (hashmap_free_t)chain_free,
};

static const hashmap_impl_t LF_CHAIN_HASHMAP_IMPL_HT = { 
  (hashmap_find_t)lf_chain_find,
  (hashmap_put_t)lf_chain_put,
  (hashmap_init_t)lf_chain_init,
  (hashmap_free_t)lf_chain_free,
};

static const hashmap_impl_t CC_HASHMAP_IMPL_HT = { 
  (hashmap_find_t)lmn_hashmap_find,
  (hashmap_put_t)lmn_hashmap_put,
  (hashmap_init_t)lmn_hashmap_init,
  (hashmap_free_t)lmn_hashmap_free,
};

void hashmap_init(hashmap_t *map, hashmap_type_t type) {
  switch (type) {
    case LMN_CLOSED_ADDRESSING:
      map->data = lmn_malloc(chain_hashmap_t);
      map->impl = CHAIN_HASHMAP_IMPL_HT;
      break;
    case LMN_LOCK_FREE_CLOSED_ADDRESSING:
      map->data = lmn_malloc(lf_chain_hashmap_t);
      map->impl = LF_CHAIN_HASHMAP_IMPL_HT;
      break;
    case LMN_MC_CLIFF_CLICK:
      map->data = lmn_malloc(lmn_hashmap_t);
      map->impl = CC_HASHMAP_IMPL_HT;
      break;
  }
  map->impl.init(map->data);
}

}
}
}

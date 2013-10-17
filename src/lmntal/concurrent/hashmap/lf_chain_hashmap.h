/**
 * @file   lf_chain_hashmap.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef LF_CHAIN_HASHMAP_H
#  define LF_CHAIN_HASHMAP_H

#include "chain_hashmap.h"

namespace lmntal {
namespace concurrent {
namespace hashmap {

void lf_chain_init(chain_hashmap_t* map);
lmn_data_t lf_chain_find(chain_hashmap_t *map, lmn_key_t key);
void lf_chain_put(chain_hashmap_t *map, lmn_key_t key, lmn_data_t data);
void lf_chain_free(chain_hashmap_t* map);

}
}
}

#endif /* ifndef LF_CHAIN_HASHMAP_H */


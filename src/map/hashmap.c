#include "hashmap.h"

void lmn_hashmap_init(lmn_hashmap_t* hashmap, const lmn_hm_type_t type) {
  switch (type) {
    case LMM_HM_OPEN_ADDRESSING:
    case LMM_HM_CLOSED_ADDRESSING:
    default:
      return;
  }
}

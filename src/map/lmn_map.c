/**
 * License GPL3
 * @file   lmn_map.c
 * @brief  abstract map structure
 * @author Taketo Yoshida
*/
#include "lmn_map.h"

void lmn_map_init(lmn_map_t* map, const lmn_map_type_t type) {
  switch (type) {
    case LMM_MAP_HASH_OPEN_ADDRESSING:
    case LMM_MAP_HASH_CLOSED_ADDRESSING:
    case LMN_MAP_HOPSCOTCH:
      break;
    default:
      return;
  }
}

/**
 * License GPL3
 * @file   lmn_map.h
 * @brief  abstract map structure
 * @author Taketo Yoshida
*/
#ifndef LMN_MAP_H
#  define LMN_MAP_H

#include "lmn_map_util.h"

typedef void*    lmn_instance_t;

typedef struct {
  lmn_instance_t   instance;
  int              (*find)(lmn_instance_t, lmn_key_t);
  int              (*put)(lmn_instance_t, lmn_key_t, lmn_data_t);
  void             (*init)(lmn_instance_t);
  void             (*free)(lmn_instance_t);
} lmn_map_t;

typedef enum {
  LMM_MAP_HASH_OPEN_ADDRESSING = 0,
  LMM_MAP_HASH_CLOSED_ADDRESSING,
  LMN_MAP_HOPSCOTCH,
  LMN_HAP_COUNT
} lmn_map_type_t;

void lmn_map_init(lmn_map_t* , const lmn_map_type_t);


#endif /* ifndef LMN_MAP_H */

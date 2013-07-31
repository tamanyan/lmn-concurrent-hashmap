/*
 * $Id$
 */
#ifndef HASHMAP_H
#  define HASHMAP_H

typedef unsigned key_t;
typedef void*    map_t;

typedef struct {
  map_t   map;
  int     (*find)(map_t, key_t);
  int     (*put)(map_t, key_t, void* data);
  void    (*init)(map_t);
  void    (*free)(map_t);
}lmn_hashmap_t;

typedef enum {
  LMM_HM_OPEN_ADDRESSING = 0,
  LMM_HM_CLOSED_ADDRESSING,
  LMN_HM_COUNT
} lmn_hm_type_t;

void lmn_hashmap_init(lmn_hashmap_t* , const lmn_hm_type_t);


#endif /* ifndef HASHMAP_H */

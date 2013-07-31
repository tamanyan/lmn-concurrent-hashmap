/*
 * $Id$
 */
#ifndef MAP_H
#  define MAP_H

typedef unsigned key_t;
typedef void*    instance_t;

typedef struct {
  instance_t   instance;
  int          (*find)(instance_t, key_t);
  int          (*put)(instance_t, key_t, void* data);
  void         (*init)(instance_t);
  void         (*free)(instance_t);
}lmn_map_t;

typedef enum {
  LMM_MAP_HASH_OPEN_ADDRESSING = 0,
  LMM_MAP_HASH_CLOSED_ADDRESSING,
  LMN_HAP_COUNT
} lmn_map_type_t;

void lmn_map_init(lmn_map_t* , const lmn_map_type_t);


#endif /* ifndef MAP_H */

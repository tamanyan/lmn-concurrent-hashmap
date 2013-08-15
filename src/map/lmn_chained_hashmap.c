/**
 * @file   lmn_chained_hashmap.c
 * @brief  
 * @author Taketo Yoshida
 */
#include "lmn_chained_hashmap.h"

#define LMN_CHAINED_INIT_SIZE (64 * 1024)

/*
 * private functions
 */
inline lmn_hash_t lmn_chained_find_bucket(lmn_chained_hashmap_t *map, lmn_key_t key) {
  lmn_hash_t i              = lmn_hash_calc(key);
  lmn_chained_entry_t **tbl = map->tbl;
  lmn_word mask             = map->bucket_mask;
}

void lmn_chained_rehash(lmn_chained_hashmap_t *map) {
  map->tbl              = lmn_malloc(LMN_CHAINED_INIT_SIZE, lmn_chained_entry_t*);
  map->bucket_mask      = LMN_CHAINED_INIT_SIZE - 1;
  map->size             = LMN_CHAINED_INIT_SIZE;
  memset(map->tbl, 0x00, LMN_CHAINED_INIT_SIZE);
}


/*
 * public functions
 */

void lmn_chained_init(lmn_chained_hashmap_t* map) {

}


lmn_data_t lmn_chained_find(lmn_chained_hashmap_t *map, lmn_key_t key) {

}

void lmn_chained_put(lmn_chained_hashmap_t *map, lmn_key_t key, lmn_data_t data) {

}

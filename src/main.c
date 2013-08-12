/**
 * @file   main.c
 * @brief  main program
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "map/lmn_map.h"
#include "map/lmn_hopscotch_hashmap.h"
#include "map/lmn_closed_hashmap.h"

#define MAX_KEY 10000000

int main(int argc, char **argv){

  /*
   * hopscotch hash table test
   */
  /*
  lmn_hopch_hashmap_t map;
  lmn_hopch_hashmap_init(&map);
  for (int i = 0; i < MAX_KEY; i++) {
    printf("put %p\n", lmn_hopch_hashmap_put(&map, (lmn_key_t)i, (lmn_data_t)i));
  }*/

  lmn_closed_hashmap_t closed_map;
  lmn_closed_init(&closed_map);
  printf("start benchmark\n");
  for (lmn_word i = 0; i < MAX_KEY; i++) {
    lmn_closed_put(&closed_map, (lmn_key_t)i, (lmn_data_t)i);
  }
  for (lmn_word i = 0; i < MAX_KEY; i++) {
    lmn_data_t data = lmn_closed_find(&closed_map, (lmn_key_t)i);
    if (data != i) {
      printf("key:%ld, value:%ld\n", i, data);
    }
  }
	return 0;
}


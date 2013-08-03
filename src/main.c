/**
 * @file   main.c
 * @brief  main program
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include "map/lmn_map.h"
#include "map/lmn_hopscotch_hashmap.h"

#define MAX_KEY 0x1000

int main(int argc, char **argv){
  lmn_hopch_hashmap_t map;

  lmn_hopch_hashmap_init(&map);
  for (int i = 0; i < MAX_KEY; i++) {
    printf("put %p\n", lmn_hopch_hashmap_put(&map, (lmn_key_t)i, (lmn_data_t)i));
  }
	return 0;
}


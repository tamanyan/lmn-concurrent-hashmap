/**
 * License GPL3
 * @file   main.c
 * @brief  main program
 * @author Taketo Yoshida
*/
#include <stdio.h>
#include "map/lmn_map.h"
#include "lmn_hopscotch_hashmap.h"

int main(int argc, char **argv){
  lmn_hopch_hashmap_t map;

  lmn_hopch_hashmap_init(&map);
	return 0;
}


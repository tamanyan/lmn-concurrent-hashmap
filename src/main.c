/**
 * @file   main.c
 * @brief  main program
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include <time.h>
#include "map/lmn_map.h"
#include "map/lmn_hopscotch_hashmap.h"
#include "map/lmn_closed_hashmap.h"
#include "map/lmn_chained_hashmap.h"

#define MAX_KEY 1000000

int main(int argc, char **argv){

  clock_t start, end;
  /*
   * hopscotch hash table test
   */
  /*
  lmn_hopch_hashmap_t map;
  lmn_hopch_hashmap_init(&map);
  for (int i = 0; i < MAX_KEY; i++) {
    printf("put %p\n", lmn_hopch_hashmap_put(&map, (lmn_key_t)i, (lmn_data_t)i));
  }*/

  lmn_data_t data;
  lmn_closed_hashmap_t closed_map;
  lmn_closed_init(&closed_map);

  int array[MAX_KEY];
  printf("start benchmark\n");

  srand((unsigned) time(NULL));
  for(int i = 0;i < 10;i++) {
    array[i] = rand(); 
  }

  start = clock();
  for (lmn_word i = 0; i < MAX_KEY; i++) {
    lmn_closed_put(&closed_map, (lmn_key_t)array[i], (lmn_data_t)i);
  }
  end = clock();
  printf("closed hash map put time:%lfs\n", (double)(end - start)/CLOCKS_PER_SEC);

  start = clock();
  for (lmn_word i = 0; i < MAX_KEY; i++) {
    data = lmn_closed_find(&closed_map, (lmn_key_t)array[i]);
  }
  end = clock();
  printf("closed hash map find time:%lfs\n", (double)(end - start)/CLOCKS_PER_SEC);

  lmn_chained_hashmap_t chained_map;
  lmn_chained_init(&chained_map);

  start = clock();
  for (lmn_word i = 0; i < MAX_KEY; i++) {
    lmn_chained_put(&chained_map, (lmn_key_t)array[i], (lmn_data_t)i);
  }
  end = clock();
  printf("chained hash map put time:%lfs\n", (double)(end - start)/CLOCKS_PER_SEC);

  start = clock();
  for (lmn_word i = 0; i < MAX_KEY; i++) {
    data = lmn_chained_find(&chained_map, (lmn_key_t)array[i]);
  }
  end = clock();
  printf("chained hash map find time:%lfs\n", (double)(end - start)/CLOCKS_PER_SEC);
	return 0;
}


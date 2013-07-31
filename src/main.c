#include <stdio.h>
#include "map/lmn_map.h"

int main(int argc, char **argv){
  lmn_map_t* test = NULL;
  printf("lmn concurrent map test\n");
  lmn_map_init(test, LMM_MAP_HASH_OPEN_ADDRESSING);
	return 0;
}


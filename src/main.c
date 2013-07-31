#include <stdio.h>
#include "map/hashmap.h"

int main(int argc, char **argv){
  lmn_hashmap_t* test = NULL;
  lmn_hashmap_init(test, LMM_HM_OPEN_ADDRESSING);
  printf("hashmap test\n");
	return 0;
}


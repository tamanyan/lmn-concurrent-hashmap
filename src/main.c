/**
 * @file   main.c
 * @brief  main program
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "map/lmn_map.h"
#include "map/lmn_hopscotch_hashmap.h"
#include "map/lmn_closed_hashmap.h"
#include "map/lmn_chained_hashmap.h"

#define MAX_KEY (100000 * HASHMAP_SEGMENT)

typedef struct {
  lmn_chained_hashmap_t  *map;
  int                  *array;
  int                      id;
  int              thread_num;
} thread_param;

void *thread(void* arg) {
  thread_param         *targ = (thread_param*)arg;
  lmn_chained_hashmap_t *map = targ->map;
  int                 *array = targ->array;
  int                     id = targ->id;
  int                    seg = (MAX_KEY / targ->thread_num);
  int                 offset = seg * id;
  lmn_data_t            data;
  printf("enter thread id:%d, offset:%d segment:%d\n", id, offset, seg);
  for (int i = offset; i < offset + seg; i++) {
    lmn_chained_put(map, (lmn_key_t)array[i], (lmn_data_t)i);
  } 
  for (int i = offset; i < offset + seg; i++) {
    data = lmn_chained_find(map, (lmn_key_t)array[i]);
  } 
}


#define ALG_NAME_LOCK_CHAINED_HASHMAP "lock_chain_hash"
#define ALG_NAME_LOCK_FREE_CHAINED_HASHMAP "lock_free_chain_hash"
#define ALG_NAME_LOCK_CLOSED_HASHMAP "lock_closed_hash"
#define ALG_NAME_LOCK_FREE_CLOSED_HASHMAP "lock_free_closed_hash"

int main(int argc, char **argv){

  clock_t      start, end;
  lmn_data_t         data;
  int              result;
  int      array[MAX_KEY];

  char      algrithm[128] = {0};
  int               count = 1;
  int          thread_num = 1;

  while((result=getopt(argc,argv,"a:c:n:"))!=-1){
    switch(result){
      case 'a':
        if (strcmp(ALG_NAME_LOCK_CHAINED_HASHMAP, optarg) == 0 ||
        strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, optarg) == 0 ||
        strcmp(ALG_NAME_LOCK_CLOSED_HASHMAP, optarg) == 0 ||
        strcmp(ALG_NAME_LOCK_FREE_CLOSED_HASHMAP, optarg) == 0) {
          strcpy(algrithm, optarg); 
        } else {
          fprintf(stderr, "unknown algrithm!! require below each names.\n");
          fprintf(stderr, "%s\n", ALG_NAME_LOCK_CHAINED_HASHMAP);
          fprintf(stderr, "%s\n", ALG_NAME_LOCK_FREE_CHAINED_HASHMAP);
          fprintf(stderr, "%s\n", ALG_NAME_LOCK_CLOSED_HASHMAP);
          fprintf(stderr, "%s\n", ALG_NAME_LOCK_FREE_CLOSED_HASHMAP);
          exit(-1);
        }
        break;
      case 'c':
      case 'n':
        thread_num = atoi(optarg);
        break;
    }
  }

  if (algrithm[0] == 0x00) {
    strcpy(algrithm, ALG_NAME_LOCK_CHAINED_HASHMAP);
  }
  srand((unsigned) time(NULL));
  for(int i = 0;i < MAX_KEY;i++) {
    array[i] = rand(); 
  }

  if (strcmp(ALG_NAME_LOCK_CHAINED_HASHMAP, algrithm) == 0) {
    pthread_t p[thread_num];
    thread_param param[thread_num];
    lmn_chained_hashmap_t chained_map;
    lmn_chained_init(&chained_map);

    start = clock();
    for (int i = 0; i < thread_num; i++) {
      param[i].map = &chained_map;
      param[i].array = array;
      param[i].id = i;
      param[i].thread_num = thread_num;
      pthread_create(&p[i] , NULL, thread, &param[i] );
    }
    for (int i = 0; i < thread_num; i++) {
      pthread_join(p[i], NULL);
    }
    end = clock();
    printf("chained hash map put find time:%lfs\n", (double)(end - start)/CLOCKS_PER_SEC);
    printf("chained map size:%d\n", chained_map.size);

    lmn_chained_entry_t *ent2;
    lmn_chained_free(&chained_map, ent2, ({
      lmn_free(ent2)      
    }));
  } else if (strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, algrithm) == 0) {
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

    lmn_chained_entry_t *ent2;
    lmn_chained_free(&chained_map, ent2, ({
      lmn_free(ent2)      
    }));
  }

  /*
  if(0) {
    lmn_closed_hashmap_t closed_map;
    lmn_closed_init(&closed_map);

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
    lmn_closed_entry_t *ent1;
    lmn_closed_free(&closed_map, ent1, ({
      lmn_free(ent1)      
    }));

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

    lmn_chained_entry_t *ent2;
    lmn_chained_free(&chained_map, ent2, ({
      lmn_free(ent2)      
    }));
  } else {
  }*/
}

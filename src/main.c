/**
 * @file   main.c
 * @brief  main program
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "map/lmn_map.h"
#include "map/lmn_hopscotch_hashmap.h"
#include "map/lmn_closed_hashmap.h"
#include "map/lmn_chained_hashmap.h"

double gettimeofday_sec(){
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)t.tv_sec + (double)t.tv_usec * 1e-6;
}

#define MAX_KEY (300000 * HASHMAP_SEGMENT)

typedef struct {
  lmn_chained_hashmap_t  *map;
  int                  *array;
  int                      id;
  int              thread_num;
} thread_chained_param;

typedef struct {
  lmn_closed_hashmap_t  *map;
  int                  *array;
  int                      id;
  int              thread_num;
} thread_closed_param;

void *thread_chained_free(void* arg) {
  thread_chained_param         *targ = (thread_chained_param*)arg;
  lmn_chained_hashmap_t *map = targ->map;
  int                 *array = targ->array;
  int                     id = targ->id;
  int                    seg = (MAX_KEY / targ->thread_num);
  int                 offset = seg * id;
  lmn_data_t            data;

  //printf("enter thread id:%d, offset:%d segment:%d\n", id, offset, seg);
  for (int i = offset; i < offset + seg; i++) {
    lmn_chained_free_put(map, (lmn_key_t)array[i], (lmn_data_t)i);
    data = lmn_chained_free_find(map, (lmn_key_t)array[i]);
    if (data != i) {
      printf("not same data\n");
    }
  } 
  //printf("finish thread id:%d, offset:%d segment:%d\n", id, offset, seg);
}

void *thread_chained(void* arg) {
  thread_chained_param         *targ = (thread_chained_param*)arg;
  lmn_chained_hashmap_t *map = targ->map;
  int                 *array = targ->array;
  int                     id = targ->id;
  int                    seg = (MAX_KEY / targ->thread_num);
  int                 offset = seg * id;
  lmn_data_t            data;
  //printf("enter thread id:%d, offset:%d segment:%d\n", id, offset, seg);
  for (int i = offset; i < offset + seg; i++) {
    lmn_chained_put(map, (lmn_key_t)array[i], (lmn_data_t)i);
    data = lmn_chained_find(map, (lmn_key_t)array[i]);
    if (data != i) {
      printf("not same data\n");
    }
  } 
  //printf("finish thread id:%d, offset:%d segment:%d\n", id, offset, seg);
}

void *thread_closed_free(void* arg) {
  thread_closed_param         *targ = (thread_closed_param*)arg;
  lmn_closed_hashmap_t *map = targ->map;
  int                 *array = targ->array;
  int                     id = targ->id;
  int                    seg = (MAX_KEY / targ->thread_num);
  int                 offset = seg * id;
  lmn_data_t            data;
  //printf("enter thread id:%d, offset:%d segment:%d\n", id, offset, seg);
  for (int i = offset; i < offset + seg; i++) {
    lmn_closed_free_put(map, (lmn_key_t)array[i], (lmn_data_t)i);
    data = lmn_closed_find(map, (lmn_key_t)array[i]);
    if (data != i) {
      printf("not same data\n");
    }
  } 
  //printf("finish thread id:%d, offset:%d segment:%d\n", id, offset, seg);
}

#define ALG_NAME_LOCK_CHAINED_HASHMAP "lock_chain_hash"
#define ALG_NAME_LOCK_FREE_CHAINED_HASHMAP "lock_free_chain_hash"
#define ALG_NAME_LOCK_CLOSED_HASHMAP "lock_closed_hash"
#define ALG_NAME_LOCK_FREE_CLOSED_HASHMAP "lock_free_closed_hash"

int main(int argc, char **argv){

  double  start, end;
  lmn_data_t         data;
  int              result;
  int              *array;

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
  array = malloc(sizeof(int) * MAX_KEY);
  for(int i = 0;i < MAX_KEY;i++) {
    array[i] = rand(); 
  }

  if (strcmp(ALG_NAME_LOCK_CHAINED_HASHMAP, algrithm) == 0) {
    pthread_t p[thread_num];
    thread_chained_param param[thread_num];
    lmn_chained_hashmap_t chained_map;
    lmn_chained_init(&chained_map);

    start = gettimeofday_sec();
    for (int i = 0; i < thread_num; i++) {
      param[i].map = &chained_map;
      param[i].array = array;
      param[i].id = i;
      param[i].thread_num = thread_num;
      pthread_create(&p[i] , NULL, thread_chained, &param[i] );
    }
    for (int i = 0; i < thread_num; i++) {
      pthread_join(p[i], NULL);
    }
    end = gettimeofday_sec();
    printf("chained hash map put find, %lf\n", end - start);
    lmn_chained_entry_t *ent2;
    lmn_chained_free(&chained_map, ent2, ({
      lmn_free(ent2)      
    }));
  } else if (strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, algrithm) == 0) {
    pthread_t p[thread_num];
    thread_chained_param param[thread_num];
    lmn_chained_hashmap_t chained_map;
    lmn_chained_init(&chained_map);

    start = gettimeofday_sec();
    for (int i = 0; i < thread_num; i++) {
      param[i].map = &chained_map;
      param[i].array = array;
      param[i].id = i;
      param[i].thread_num = thread_num;
      pthread_create(&p[i] , NULL, thread_chained_free, &param[i] );
    }
    for (int i = 0; i < thread_num; i++) {
      pthread_join(p[i], NULL);
    }
    end = gettimeofday_sec();
    printf("lock-free chained hash map put find, %lf\n", end - start);
    lmn_chained_entry_t *ent2;
    lmn_chained_free(&chained_map, ent2, ({
      lmn_free(ent2)      
    }));
  } else if (strcmp(ALG_NAME_LOCK_FREE_CLOSED_HASHMAP, algrithm) == 0) {
    pthread_t p[thread_num];
    thread_closed_param param[thread_num];
    lmn_closed_hashmap_t closed_map;
    lmn_closed_init(&closed_map);

    start = gettimeofday_sec();
    for (int i = 0; i < thread_num; i++) {
      param[i].map = &closed_map;
      param[i].array = array;
      param[i].id = i;
      param[i].thread_num = thread_num;
      pthread_create(&p[i] , NULL, thread_closed_free, &param[i] );
    }
    for (int i = 0; i < thread_num; i++) {
      pthread_join(p[i], NULL);
    }
    end = gettimeofday_sec();
    printf("lock-free closed hash map put find, %lf\n", end - start);
    lmn_closed_entry_t *ent2;
    lmn_closed_free(&closed_map, ent2, ({
      lmn_free(ent2)      
    }));
  }
}

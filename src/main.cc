/**
 * @file   main.cc
 * @brief  main program
 * @author Taketo Yoshida
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "map/lmn_map.h"
#include "map/lmn_closed_hashmap.h"
#include "map/lmn_chained_hashmap.h"
#include "lmntal/concurrent/hashmap/hashmap.hpp"
#include "lmntal/concurrent/hashmap/chain_hashmap.hpp"
#include "lmntal/concurrent/thread.h"
#include <unistd.h>
#include <iostream>
using namespace std;
using namespace lmntal::concurrent::hashmap;
using namespace lmntal::concurrent;

double gettimeofday_sec(){
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)t.tv_sec + (double)t.tv_usec * 1e-6;
}

#define MAX_KEY (300000 * 12)

#define ALG_NAME_LOCK_CHAINED_HASHMAP "lock_chain_hash"
#define ALG_NAME_LOCK_FREE_CHAINED_HASHMAP "lock_free_chain_hash"
#define ALG_NAME_LOCK_CLOSED_HASHMAP "lock_closed_hash"
#define ALG_NAME_LOCK_FREE_CLOSED_HASHMAP "lock_free_closed_hash"

__thread int _id;

class HashMapTest : public Thread {
private:
  static int count;
  int id;
public:
  HashMap<int, int> *map;
  
  HashMapTest() : id(HashMapTest::count++), Runnable() {}

  void initialize(HashMap<int, int> *hashmap) {
    map = hashmap;
  }

  void Run() {
    int section = (MAX_KEY / HashMapTest::count);
    int offset  = section * id;
    //printf("Enter id: %d offset:%d last:%d\n",id, offset, offset+section-1);
    for (int i = offset; i < offset + section; i++) {
      map->Put(i, i);
    }
    ;
    printf("End id: %d\n", GetCurrentThreadId());
  }
};

int HashMapTest::count = 0;

typedef struct {
  lmn_chained_hashmap_t  *map;
  int                  *array;
  int                      id;
  int              thread_num;
} thread_chained_param;

void *thread_chained_free(void* arg) {
  thread_chained_param         *targ = (thread_chained_param*)arg;
  lmn_chained_hashmap_t *map = targ->map;
  int                     id = targ->id;
  int                    seg = (MAX_KEY / targ->thread_num);
  int                 offset = seg * id;
  lmn_data_t            data;

  printf("enter thread id:%d, offset:%d last:%d\n", id, offset, offset+seg-1);
  for (int i = offset; i < offset + seg; i++) {
    lmn_chained_free_put(map, (lmn_key_t)i, (lmn_data_t)i);
    /*data = lmn_chained_free_find(map, (lmn_key_t)array[i]);
    if (data != i) {
      printf("not same data\n");
    }*/
  } 
  for (int i = offset; i < offset + seg; i++) {
    data = lmn_chained_free_find(map, (lmn_key_t)i);
    if (data != (lmn_data_t)i) {
      printf("not same data\n");
    }
  }
}

int main(int argc, char **argv){

  double  start, end;
  lmn_data_t         data;
  int              result;
  int              *array;
  extern char *optarg;
  extern int  optind, opterr;

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
  HashMap<int, int> *map;

  if (strcmp(ALG_NAME_LOCK_CHAINED_HASHMAP, algrithm) == 0) {
    map = new ConcurrentChainHashMap<int, int>();
    cout << "ConcurrentChainHashMap" << endl;
  } else if (strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, algrithm) == 0) {
    map = new LockFreeChainHashMap<int, int>(thread_num);
    cout << "LockFreeChainHashMap" << endl;
  } else if (strcmp(ALG_NAME_LOCK_FREE_CLOSED_HASHMAP, algrithm) == 0) {
  }
  HashMapTest *threads = new HashMapTest[thread_num];
  for (int i = 0; i < thread_num; i++) {
    threads[i].initialize(map);
    threads[i].Start();
  }
  for (int i = 0; i < thread_num; i++) {
    threads[i].Join();
  }
  for (int i = 0; i < MAX_KEY; i++) {
    if (!map->Exist(i))
      cout << "not found " << i << endl;
  }
  cout << map->Count() << endl;

  //pthread_t p[thread_num];
  //thread_chained_param param[thread_num];
  //lmn_chained_hashmap_t chained_map;
  //lmn_chained_init(&chained_map);

  //start = gettimeofday_sec();
  //for (int i = 0; i < thread_num; i++) {
  //  param[i].map = &chained_map;
  //  param[i].id = i;
  //  param[i].thread_num = thread_num;
  //  pthread_create(&p[i] , NULL, thread_chained_free, &param[i] );
  //}
  //for (int i = 0; i < thread_num; i++) {
  //  pthread_join(p[i], NULL);
  //}
  //end = gettimeofday_sec();
  //printf("lock-free chained hash map put find, %lf\n", end - start);
  //lmn_chained_entry_t *ent2;
  //lmn_chained_free(&chained_map, ent2, ({
  //  lmn_free(ent2)      
  //}));
  delete map;
}

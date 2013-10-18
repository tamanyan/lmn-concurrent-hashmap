/**
 * @file   main.cc
 * @brief  main program
 * @author Taketo Yoshida
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include "lmntal/concurrent/hashmap/chain_hashmap.h"
#include "lmntal/concurrent/hashmap/lf_chain_hashmap.h"
#include "lmntal/concurrent/hashmap/cc_hashmap.h"
#include "lmntal/concurrent/thread.h"
#include <iostream>
#include <time.h>

#define uint64_t long long

using namespace std;
using namespace lmntal::concurrent::hashmap;
using namespace lmntal::concurrent;

double gettimeofday_sec(){
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)t.tv_sec + (double)t.tv_usec * 1e-6;
}

#define MAX_KEY (300000 * 12)
#define COUNT (1 << 17)

#define ALG_NAME_LOCK_CHAINED_HASHMAP "lch"
#define ALG_NAME_LOCK_FREE_CHAINED_HASHMAP "lfch"
#define ALG_NAME_CC_HASHMAP "cch"

static int num_threads_;
static volatile int start_, stop_, load_;
static double load_time_;
static int duration_;

double second_high_resolution(clockid_t clockid) {
  timespec ts;
  clock_gettime(clockid, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

class HashMapTest : public Thread {
private:
  static int count;
  int id;
public:
  hashmap_t* map;
  double cpu_time;
  
  HashMapTest() : cpu_time(0), id(HashMapTest::count++), Runnable() {}

  void initialize(hashmap_t *hashmap) {
    map = hashmap;
    srand((unsigned)time(NULL));
  }

  void Run() {
    int section = (COUNT / HashMapTest::count);
    int offset  = section * id + 1; 
    double test = 0;
    printf("Enter section: %d \n",section);
    do {} while(start_);
    clockid_t clock;
    double start, end;
    if (pthread_getcpuclockid(pthread_self(), &clock) != 0){
      printf("error\n");
    }
    start = second_high_resolution(clock);
    for (int i = offset; i < offset + section; i++) {
    //for (int i = 0; i < COUNT; i++) {
      hashmap_put(map, i, (lmn_data_t)i);
    }
    end = second_high_resolution(clock);
    cpu_time = end - start;
    printf("End id: %d, cpu_time:%lf\n", id, cpu_time);
  }
};

int HashMapTest::count = 0;

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
        strcmp(ALG_NAME_CC_HASHMAP, optarg) == 0) {
          strcpy(algrithm, optarg); 
        } else {
          fprintf(stderr, "unknown algrithm!! require below each names.\n");
          fprintf(stderr, "lock based chain hashmap : %s\n", ALG_NAME_LOCK_CHAINED_HASHMAP);
          fprintf(stderr, "%s\n", ALG_NAME_LOCK_FREE_CHAINED_HASHMAP);
          fprintf(stderr, "%s\n", ALG_NAME_CC_HASHMAP);
          exit(-1);
        }
        break;
      case 'c':
      case 'n':
        num_threads_ = thread_num = atoi(optarg);
        break;
    }
  }
  if (algrithm[0] == 0x00) {
    strcpy(algrithm, ALG_NAME_LOCK_CHAINED_HASHMAP);
  }

  hashmap_t map;
  if (strcmp(ALG_NAME_LOCK_CHAINED_HASHMAP, algrithm) == 0) {
    cout << "ConcurrentChainHashMap" << endl;
    hashmap_init(&map, LMN_CLOSED_ADDRESSING);
  } else if (strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, algrithm) == 0) {
    cout << "LockFreeChainHashMap" << endl;
    hashmap_init(&map, LMN_LOCK_FREE_CLOSED_ADDRESSING);
  } else if (strcmp(ALG_NAME_CC_HASHMAP, algrithm) == 0) {
    cout << "Cliff Click HashMap For Model Checking" << endl;
    hashmap_init(&map, LMN_MC_CLIFF_CLICK);
  }
  if (map.data) {
    HashMapTest *threads = new HashMapTest[thread_num];
    for (int i = 0; i < thread_num; i++) {
      threads[i].initialize(&map);
    }
    start_ = 0;
    for (int i = 0; i < thread_num; i++) {
      threads[i].Start();
    }
    usleep(100000);
    start_ = 1;
    double start = gettimeofday_sec();
    double cpu_time = 0;
    for (int i = 0; i < thread_num; i++) {
      threads[i].Join();
      cpu_time += threads[i].cpu_time;
    }
    cpu_time /= thread_num;
    double end = gettimeofday_sec();
    load_time_ = end - start;
    printf("Mops/s %lf per-thread %lf\n", ((double)COUNT / cpu_time) / 1000000.0 , ((double)COUNT/cpu_time) / 1000000.0);
    hashmap_free(&map);
    //for (int i = 0; i < MAX_KEY; i++) {
    //  if (!map->st(i))
    //    cout << "not found " << i << endl;
    //}
    //cout << map->Count() << endl;
  }
}

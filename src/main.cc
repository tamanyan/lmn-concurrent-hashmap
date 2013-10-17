/**
 * @file   main.cc
 * @brief  main program
 * @author Taketo Yoshida
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "lmntal/concurrent/hashmap/chain_hashmap.h"
#include "lmntal/concurrent/hashmap/lf_chain_hashmap.h"
#include "lmntal/concurrent/hashmap/cc_hashmap.h"
#include "lmntal/concurrent/thread.h"
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

#define ALG_NAME_LOCK_CHAINED_HASHMAP "lch"
#define ALG_NAME_LOCK_FREE_CHAINED_HASHMAP "lfch"
#define ALG_NAME_CC_HASHMAP "cch"

static int num_threads_;
static volatile int start_, stop_, load_;
static hashmap_t map_;
static double load_time_;
static int duration_;

class HashMapTest : public Thread {
private:
  static int count;
  int id;
public:
  hashmap_t* map;
  int ops_count;
  
  HashMapTest() : ops_count(0), id(HashMapTest::count++), Runnable() {}

  void initialize(hashmap_t *hashmap) {
    map = hashmap;
  }

  void Run() {
    int rand_val;
    printf("Enter id: %d \n",id);
    srand((unsigned)time(NULL));
    do {} while(start_);
    while(!stop_) {
      rand_val = rand();
      ops_count++;
      //hashmap_put(map, rand_val, (lmn_data_t)rand_val);
      count++;
    }
    printf("End id: %d\n", GetCurrentThreadId());
  }
};

int HashMapTest::count = 0;

static volatile int test;

void *worker (void *arg) {
    // Wait for all the worker threads to be ready.
    (void)LMN_ATOMIC_ADD(&load_, -1);
    do {} while (load_);

    // Wait for all the worker threads to be done loading.
    (void)LMN_ATOMIC_ADD(&start_, -1);
    do {} while (start_);

    uint64_t ops = 0;
    int rand_val = 0;
    while (!stop_) {
        ++ops;
        hashmap_put(&map_, rand_val, (lmn_data_t)rand_val);
    }

    return (void *)ops;
}

uint64_t run_test (void) {
    load_ = num_threads_ + 1;
    start_ = num_threads_ + 1;

    stop_ = 0;

    pthread_t thread[num_threads_];
    for (int i = 0; i < num_threads_; ++i) {
        int rc = pthread_create(thread + i, NULL, worker, (void*)(size_t)i);
        if (rc != 0) { perror("pthread_create"); exit(rc); }
    }

    do { /* nothing */ } while (load_ != 1);
    load_ = 0;

    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

    do { /* nothing */ } while (start_ != 1);

    gettimeofday(&tv2, NULL);
    load_time_ = (double)(1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec) / 1000000;

    start_ = 0;
    sleep(duration_);
    stop_ = 1;

    uint64_t ops = 0;
    for (int i = 0; i < num_threads_; ++i) {
        void *count;
        pthread_join(thread[i], &count);
        ops += (size_t)count;
    }
    return ops;
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

  if (strcmp(ALG_NAME_LOCK_CHAINED_HASHMAP, algrithm) == 0) {
    cout << "ConcurrentChainHashMap" << endl;
    hashmap_init(&map_, LMN_CLOSED_ADDRESSING);
  } else if (strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, algrithm) == 0) {
    cout << "LockFreeChainHashMap" << endl;
    hashmap_init(&map_, LMN_LOCK_FREE_CLOSED_ADDRESSING);
  } else if (strcmp(ALG_NAME_CC_HASHMAP, algrithm) == 0) {
    cout << "Cliff Click HashMap For Model Checking" << endl;
    hashmap_init(&map_, LMN_MC_CLIFF_CLICK);
  }
  srand((unsigned)time(NULL));
  duration_ = 3;
  double mops_per_sec = (double)run_test() / 1000000.0 / duration_;

  hashmap_free(&map_);
  printf("Threads:%-2d  load time:%-4.2f  Mops/s:%-4.2f  per-thread:%-4.2f \n",
      num_threads_, load_time_, mops_per_sec, mops_per_sec/num_threads_);
  //if (map.data) {
  //  HashMapTest *threads = new HashMapTest[thread_num];
  //  start_ = 1;
  //  for (int i = 0; i < thread_num; i++) {
  //    threads[i].initialize(&map);
  //  }
  //  for (int i = 0; i < thread_num; i++) {
  //    threads[i].Start();
  //  }

  //  for (int i = 0; i < thread_num; i++) {
  //    threads[i].Join();
  //    ops_count += threads[i].ops_count;
  //  }
  //  printf("ops_count:%d, %.1lf Mops/s %lf per-thread\n", ops_count, (float)ops_count/(during*1000000.0), (float)ops_count/(during*1000000.0));
  //  //for (int i = 0; i < MAX_KEY; i++) {
  //  //  if (!map->st(i))
  //  //    cout << "not found " << i << endl;
  //  //}
  //  //cout << map->Count() << endl;
  //}
}

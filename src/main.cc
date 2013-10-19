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

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static __thread unsigned long mt[N]; /* the array for the state vector  */
static __thread int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void init_genrand(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
      (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
long genrand_int31(void)
{
    return (long)(genrand_int32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double genrand_real1(void)
{
    return genrand_int32()*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double genrand_real2(void)
{
    return genrand_int32()*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double genrand_real3(void)
{
    return (((double)genrand_int32()) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(void) 
{ 
    unsigned long a=genrand_int32()>>5, b=genrand_int32()>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */

#define MAX_KEY (300000 * 12)
#define COUNT (1 << 17)

#define ALG_NAME_LOCK_CHAINED_HASHMAP "lch"
#define ALG_NAME_LOCK_FREE_CHAINED_HASHMAP "lfch"
#define ALG_NAME_CC_HASHMAP "cch"

static int num_threads_;
static volatile int start_, stop_, load_;
static double load_time_;
static int duration_;

class HashMapTest : public Thread {
private:
  static int count;
  int id;
public:
  hashmap_t* map;
  double cpu_time;
  int ops;
  
  HashMapTest() : ops(0), cpu_time(0), id(HashMapTest::count++), Runnable() {}

  void initialize(hashmap_t *hashmap) {
    map = hashmap;
  }

  void Run() {
    int section = (COUNT / HashMapTest::count);
    int offset  = section * id + 1; 
    init_genrand((unsigned)time(NULL));
    //LMN_DBG_V("Enter section: %d \n",section);
    do {} while(start_);
    //for (int i = offset; i < offset + section; i++) {
    //  //LMN_ASSERT(!(hashmap_find(map, i) == (lmn_data_t)i));
    //  if (!(hashmap_find(map, i) == (lmn_data_t)i)) {
    //  }
    //}
    int insert_count = 0;
    int rand_val = genrand_int32();
    while(stop_ == 0) {
      this->ops++;
      for (int i = 0; i < 4000; i++)
        rand_val = genrand_int32();
      if (hashmap_find(map, rand_val) == (lmn_data_t)-1) {
        insert_count++;
        //hashmap_put(map, rand_val, (lmn_data_t)rand_val);
        //printf("insert :%d\n", hashmap_find(map, rand_val));
        //LMN_ASSERT(hashmap_find(map, rand_val) != (lmn_data_t)-1);
      }
    }
    //LMN_DBG_V("End id: %d, insert_time:%d, ops:%d\n", id, insert_count, ops);
  }
};

int HashMapTest::count = 0;

#define U_SEC 1000000

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
          fprintf(stderr, "Lock Based Chain HashMap : %s\n", ALG_NAME_LOCK_CHAINED_HASHMAP);
          fprintf(stderr, "%s\n", ALG_NAME_LOCK_FREE_CHAINED_HASHMAP);
          fprintf(stderr, "Cliff Click Hash Table for Model Checking %s\n", ALG_NAME_CC_HASHMAP);
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
    LMN_DBG("ConcurrentChainHashMap\n");
    hashmap_init(&map, LMN_CLOSED_ADDRESSING);
  } else if (strcmp(ALG_NAME_LOCK_FREE_CHAINED_HASHMAP, algrithm) == 0) {
    LMN_DBG("LockFreeChainHashMap\n");
    hashmap_init(&map, LMN_LOCK_FREE_CLOSED_ADDRESSING);
  } else if (strcmp(ALG_NAME_CC_HASHMAP, algrithm) == 0) {
    LMN_DBG("Cliff Click HashMap For Model Checking\n");
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
    usleep(500000);
    start_ = 1;
    int ops = 0;
    int during = 100000;
    usleep(during);
    stop_ = 1;
    for (int i = 0; i < thread_num; i++) {
      threads[i].Join();
      ops += threads[i].ops;
    }
    printf("ops %d\n", ops);
    //printf("%lfs Mops/s %lf per-thread %lf\n", cpu_time, ((double)COUNT / cpu_time) / 1000000.0 , ((double)COUNT/cpu_time/thread_num) / 1000000.0);
    printf("%lf s, %.3lf Mops/s,  per-thread %.3lf\n", ((double)during/U_SEC), ((double)ops / ((double)during/U_SEC)) / 1000000.0, ((double)ops / ((double)during/U_SEC)) / 1000000.0 / thread_num );
    //printf("%lfs Mops/s %lf per-thread %lf\n", during, ((double)ops/ during) / 1000000.0 , ((double)ops/during) / 1000000.0);
    hashmap_free(&map);
  }
}

/**
 * @file   thread.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef THREAD_H
#  define THREAD_H

#include <pthread.h>
#include <iostream>
using namespace std;

extern __thread int _thread_id;

namespace lmntal {
namespace concurrent {

int GetCurrentThreadId();
int GetCurrentThreadCount();

class Runnable {
private:
protected:
public:
  Runnable() {}
  virtual ~Runnable() {}
  virtual void Run() = 0;
};

class Thread : virtual public Runnable {
private:
  Runnable *runnable;
  pthread_t threadID;
  
public:
  Thread() : runnable(NULL) {}
  
  Thread(Runnable *runnable) {
    this->runnable = runnable;
  }
  int Start();
  int Join();
};

}
}

#endif /* ifndef THREAD_H */


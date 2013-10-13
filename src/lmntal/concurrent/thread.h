/**
 * @file   thread.h
 * @brief  
 * @author Taketo Yoshida
 */
#ifndef THREAD_H
#  define THREAD_H

#include <pthread.h>

namespace lmntal {
namespace concurrent {

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
  
  static void *__Run(void *cthis) {
    static_cast<Runnable*>(cthis)->Run();
    return NULL;
  }
  
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


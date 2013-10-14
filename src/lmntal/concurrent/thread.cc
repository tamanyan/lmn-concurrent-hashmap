/**
 * @file   thread.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "thread.h"
#include <stdio.h>

__thread int _thread_id = 0;

namespace lmntal {
namespace concurrent {

int _thread_count = 0;
__thread int _thread_id;

int GetCurrentThreadId() {
  return _thread_id;
}

void* __Run(void *cthis) {
  _thread_id = _thread_count++;
  static_cast<Runnable*>(cthis)->Run();
  return NULL;
}

int Thread::Start() {
  Runnable *execRunnable = this;
  if(this->runnable != NULL){
    execRunnable = this->runnable;
  }
  return pthread_create(&threadID, NULL, __Run ,execRunnable);
}

int Thread::Join() {
  return pthread_join(threadID, NULL);
}

}
}

/**
 * @file   thread.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "thread.h"
#include <stdio.h>


namespace lmntal {
namespace concurrent {

int _thread_count = -1;
__thread int _thread_id = 0;

int GetCurrentThreadId() {
  return _thread_id;
}

int GetCurrentThreadCount() {
  return _thread_count + 1;
}

void* __Run(void *cthis) {
  __sync_fetch_and_add(&_thread_count, 1);
  _thread_id = _thread_count;
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

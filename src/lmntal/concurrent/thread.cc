/**
 * @file   thread.cc
 * @brief  
 * @author Taketo Yoshida
 */
#include "thread.h"

namespace lmntal {
namespace concurrent {

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

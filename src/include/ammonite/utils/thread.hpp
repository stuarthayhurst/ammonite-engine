#ifndef AMMONITETHREAD
#define AMMONITETHREAD

#include <climits>
#include <semaphore>

//Functions of this type must not block conditionally on other jobs
using AmmoniteWork = void (*)(void* userPtr);

using AmmoniteGroup = std::counting_semaphore<INT_MAX>;

namespace ammonite {
  namespace utils {
    namespace thread {
      unsigned int getHardwareThreadCount();
      unsigned int getThreadPoolSize();

      bool createThreadPool(unsigned int threadCount);
      void destroyThreadPool();

      void submitWork(AmmoniteWork work, void* userPtr);
      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group);
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteGroup* group, unsigned int jobCount,
                          AmmoniteGroup* submitGroup);
      void submitMultipleSync(AmmoniteWork work, void* userBuffer, int stride,
                              AmmoniteGroup* group, unsigned int jobCount);
      void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount);

      void blockThreads();
      void unblockThreads();
      void finishWork();
    }
  }
}

#endif


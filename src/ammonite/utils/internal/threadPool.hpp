#ifndef THREADMANAGER
#define THREADMANAGER

#include "../../types.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      namespace internal {
        unsigned int getHardwareThreadCount();
        unsigned int getThreadPoolSize();

        bool createThreadPool(unsigned int extraThreads);
        void destroyThreadPool();

        void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group);
        void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                            AmmoniteGroup* group, unsigned int jobCount);

        void blockThreads();
        void unblockThreads();
        void finishWork();
      }
    }
  }
}

#endif


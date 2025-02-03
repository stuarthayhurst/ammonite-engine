#ifndef THREADMANAGER
#define THREADMANAGER

#include "../internal.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      namespace AMMONITE_INTERNAL internal {
        unsigned int getHardwareThreadCount();
        unsigned int getThreadPoolSize();

        bool createThreadPool(unsigned int threadCount);
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

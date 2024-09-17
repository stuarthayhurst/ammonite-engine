#ifndef THREADMANAGER
#define THREADMANAGER

#include "../../types.hpp"

namespace ammonite {
  namespace thread {
    namespace internal {
      unsigned int getHardwareThreadCount();
      unsigned int getThreadPoolSize();

      int createThreadPool(unsigned int extraThreads);
      void destroyThreadPool();

      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion);
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteCompletion* completions, unsigned int jobCount);

      void blockThreads(bool sync);
      void unblockThreads(bool sync);
      void finishWork();
    }
  }
}

#ifdef DEBUG
namespace ammonite {
  namespace thread {
    namespace internal {
      bool debugCheckRemainingWork(bool verbose);
    }
  }
}
#endif

#endif

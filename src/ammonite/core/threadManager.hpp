#ifndef THREADMANAGER
#define THREADMANAGER

#include <atomic>

#include "../types.hpp"

namespace ammonite {
  namespace thread {
    namespace internal {
      unsigned int getHardwareThreadCount();
      unsigned int getThreadPoolSize();

      int createThreadPool(unsigned int extraThreads);
      void destroyThreadPool();

      void submitWork(AmmoniteWork work, void* userPtr, std::atomic_flag* completion);
      void submitMultiple(AmmoniteWork work, int jobCount);
      void submitMultipleUser(AmmoniteWork work, void** userPtrs, int jobCount);
      void submitMultipleComp(AmmoniteWork work, std::atomic_flag* completions, int jobCount);
      void submitMultipleUserComp(AmmoniteWork work, void** userPtrs, std::atomic_flag* completions,
                                  int jobCount);

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

#ifndef THREAD
#define THREAD

#include "../types.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      unsigned int getThreadPoolSize();
      void submitWork(AmmoniteWork work, void* userPtr);
      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion);
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteGroup* group, unsigned int jobCount);
      void waitWorkComplete(AmmoniteCompletion* completion);
      void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount);

      void blockThreads();
      void unblockThreads();
      void finishWork();
    }
  }
}

#endif


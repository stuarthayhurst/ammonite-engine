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
      void waitWorkCompleteUnsafe(AmmoniteCompletion* completion);
      void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount);
      void waitGroupCompleteUnsafe(AmmoniteGroup* group, unsigned int jobCount);
      void resetCompletion(AmmoniteCompletion* completion);
      void resetCompletionUnsafe(AmmoniteCompletion* completion);

      void blockThreads();
      void unblockThreads();
      void finishWork();
    }
  }
}

#endif


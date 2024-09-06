#ifndef THREAD
#define THREAD

#include "types.hpp"

namespace ammonite {
  namespace thread {
    unsigned int getThreadPoolSize();
    void submitWork(AmmoniteWork work, void* userPtr);
    void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion);
    void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                        AmmoniteCompletion* completions, int jobCount);
    void waitWorkComplete(AmmoniteCompletion* completion);
    void waitWorkCompleteUnsafe(AmmoniteCompletion* completion);
    void resetCompletion(AmmoniteCompletion* completion);
    void resetCompletionUnsafe(AmmoniteCompletion* completion);

    void blockThreadsAsync();
    void blockThreadsSync();
    void unblockThreadsSync();
    void unblockThreadsAsync();
    void finishWork();
  }
}

#endif

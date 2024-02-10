#ifndef THREAD
#define THREAD

#include <atomic>

namespace ammonite {
  namespace thread {
    unsigned int getThreadPoolSize();
    void submitWork(AmmoniteWork work, void* userPtr);
    void submitWork(AmmoniteWork work, void* userPtr, std::atomic_flag* completion);
    void submitMultiple(AmmoniteWork work, void** userPtrs,
                        std::atomic_flag* completions, int jobCount);
    void waitWorkComplete(std::atomic_flag* completion);

    void blockThreadsAsync();
    void blockThreadsSync();
    void unblockThreadsSync();
    void unblockThreadsAsync();
    void finishWork();
  }
}

#endif

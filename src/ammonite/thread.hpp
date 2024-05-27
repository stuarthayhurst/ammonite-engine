#ifndef THREAD
#define THREAD

#include <atomic>

#include "types.hpp"

namespace ammonite {
  namespace thread {
    unsigned int getThreadPoolSize();
    void submitWork(AmmoniteWork work, void* userPtr);
    void submitWork(AmmoniteWork work, void* userPtr, std::atomic_flag* completion);
    void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
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

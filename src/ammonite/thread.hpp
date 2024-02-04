#ifndef THREAD
#define THREAD

#include <atomic>

namespace ammonite {
  namespace thread {
    unsigned int getThreadPoolSize();
    void submitWork(AmmoniteWork work, void* userPtr);
    void submitWork(AmmoniteWork work, void* userPtr, std::atomic_flag* completion);
    void waitWorkComplete(std::atomic_flag* completion);

    void blockThreadsAsync();
    void blockThreadsSync();
    void unblockThreadsSync();
    void unblockThreadsAsync();
  }
}

#endif

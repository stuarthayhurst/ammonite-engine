#include <atomic>

#include "core/threadManager.hpp"

namespace ammonite {
  namespace thread {
    int getThreadPoolSize() {
      return internal::getThreadPoolSize();
    }

    void submitWork(AmmoniteWork work, void* userPtr) {
      internal::submitWork(work, userPtr, nullptr);
    }

    void submitWork(AmmoniteWork work, void* userPtr, std::atomic_flag* completion) {
      internal::submitWork(work, userPtr, completion);
    }

    //userPtrs and completions may be null
    void submitMultiple(AmmoniteWork work, void** userPtrs, int stride,
                        std::atomic_flag* completions, int jobCount) {
      ammonite::thread::internal::submitMultiple(work, userPtrs, stride, completions, jobCount);
    }

    void waitWorkComplete(std::atomic_flag* completion) {
      //Wait for completion to become true
      if (completion != nullptr) {
        completion->wait(false);
      }
    }

    void blockThreadsAsync() {
      internal::blockThreads(false);
    }

    void blockThreadsSync() {
      internal::blockThreads(true);
    }

    void unblockThreadsAsync() {
      internal::unblockThreads(false);
    }

    void unblockThreadsSync() {
      internal::unblockThreads(true);
    }

    void finishWork() {
      internal::finishWork();
    }
  }
}

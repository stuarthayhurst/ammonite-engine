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

    void submitMultiple(AmmoniteWork work, void** userPtrs,
                        std::atomic_flag* completions, int jobCount) {
      const int threadCount = getThreadPoolSize();

      int offset = 0;
      for (int i = 0; i < jobCount / threadCount; i++) {
        if (userPtrs == nullptr) {
          if (completions == nullptr) {
            ammonite::thread::internal::submitMultiple(work, threadCount);
          } else {
            ammonite::thread::internal::submitMultipleComp(work, completions + offset, threadCount);
          }
        } else {
          if (completions == nullptr) {
            ammonite::thread::internal::submitMultipleUser(work, userPtrs + offset, threadCount);
          } else {
            ammonite::thread::internal::submitMultipleUserComp(work, userPtrs + offset,
                                                               completions + offset, threadCount);
          }
        }
        offset += threadCount;
      }

      int remainingJobs = jobCount % threadCount;
      if (userPtrs == nullptr) {
        if (completions == nullptr) {
          ammonite::thread::internal::submitMultiple(work, remainingJobs);
        } else {
          ammonite::thread::internal::submitMultipleComp(work, completions + offset, remainingJobs);
        }
      } else {
        if (completions == nullptr) {
          ammonite::thread::internal::submitMultipleUser(work, userPtrs + offset, remainingJobs);
        } else {
          ammonite::thread::internal::submitMultipleUserComp(work, userPtrs + offset,
                                                             completions + offset, remainingJobs);
        }
      }
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
  }
}

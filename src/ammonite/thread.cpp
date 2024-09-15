#include "types.hpp"
#include "core/threadManager.hpp"

namespace ammonite {
  namespace thread {
    unsigned int getThreadPoolSize() {
      return internal::getThreadPoolSize();
    }

    void submitWork(AmmoniteWork work, void* userPtr) {
      internal::submitWork(work, userPtr, nullptr);
    }

    void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion) {
      internal::submitWork(work, userPtr, completion);
    }

    //userBuffer and completions may be null
    void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                        AmmoniteCompletion* completions, int jobCount) {
      ammonite::thread::internal::submitMultiple(work, userBuffer, stride, completions, jobCount);
    }

    //Wait for completion to be finished
    void waitWorkComplete(AmmoniteCompletion* completion) {
      //Wait for completion to become true
      if (completion != nullptr) {
        completion->wait(false);
      }
    }

    //Wait for completion to be finished, without a nullptr check
    void waitWorkCompleteUnsafe(AmmoniteCompletion* completion) {
      //Wait for completion to become true
      completion->wait(false);
    }

    //Reset completion to be used again
    void resetCompletion(AmmoniteCompletion* completion) {
      //Reset the completion to false
      if (completion != nullptr) {
        completion->clear();
      }
    }

    //Reset completion to be used again, without a nullptr check
    void resetCompletionUnsafe(AmmoniteCompletion* completion) {
      //Reset the completion to false
      completion->clear();
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

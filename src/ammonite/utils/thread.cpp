#include "../types.hpp"
#include "../core/threadManager.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      unsigned int getThreadPoolSize() {
        return ammonite::thread::internal::getThreadPoolSize();
      }

      void submitWork(AmmoniteWork work, void* userPtr) {
        ammonite::thread::internal::submitWork(work, userPtr, nullptr);
      }

      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion) {
        ammonite::thread::internal::submitWork(work, userPtr, completion);
      }

      //userBuffer and completions may be null
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteCompletion* completions, unsigned int jobCount) {
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
        ammonite::thread::internal::blockThreads(false);
      }

      void blockThreadsSync() {
        ammonite::thread::internal::blockThreads(true);
      }

      void unblockThreadsAsync() {
        ammonite::thread::internal::unblockThreads(false);
      }

      void unblockThreadsSync() {
        ammonite::thread::internal::unblockThreads(true);
      }

      void finishWork() {
        ammonite::thread::internal::finishWork();
      }
    }
  }
}

#include "../types.hpp"
#include "internal/threadManager.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      /*
       - Return the number of threads in the pool
          - Returns 0 if the pool doesn't exist
      */
      unsigned int getThreadPoolSize() {
        return ammonite::thread::internal::getThreadPoolSize();
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer
         - userPtr may be a nullptr
      */
      void submitWork(AmmoniteWork work, void* userPtr) {
        ammonite::thread::internal::submitWork(work, userPtr, nullptr);
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer and completion
         - If completion isn't a nullptr, it should be initialised with ATOMIC_FLAG_INIT
         - userPtr may be a nullptr
      */
      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion) {
        ammonite::thread::internal::submitWork(work, userPtr, completion);
      }

      /*
       - Submit multiple jobs to the thread pool, with a user-provided buffer and completions
         - completions should either be a nullptr or an array of completions
           - These should be initialised with ATOMIC_FLAG_INIT
         - userBuffer should either be a nullptr, or an array of data to be split between jobs
           - Each job will receive a section according to (userBuffer + job index * stride)
           - stride should by the size of each section to give to a job, in bytes
         - jobCount specifies how many times submit the job
      */
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

      //Block the thread pool from starting newer jobs, return immediately
      void blockThreadsAsync() {
        ammonite::thread::internal::blockThreads(false);
      }

      //Block the thread pool from starting newer jobs, return after it takes effect
      void blockThreadsSync() {
        ammonite::thread::internal::blockThreads(true);
      }

      //Unblock the thread pool from starting newer jobs, return immediately
      void unblockThreadsAsync() {
        ammonite::thread::internal::unblockThreads(false);
      }

      //Unblock the thread pool from starting newer jobs, return after it takes effect
      void unblockThreadsSync() {
        ammonite::thread::internal::unblockThreads(true);
      }

      //Wait until all work in the pool as of the call is finished
      void finishWork() {
        ammonite::thread::internal::finishWork();
      }
    }
  }
}

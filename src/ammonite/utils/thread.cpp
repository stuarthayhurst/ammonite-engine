#include "../types.hpp"
#include "internal/threadPool.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      /*
       - Return the number of threads in the pool
          - Returns 0 if the pool doesn't exist
      */
      unsigned int getThreadPoolSize() {
        return ammonite::utils::thread::internal::getThreadPoolSize();
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer
         - userPtr may be a nullptr
      */
      void submitWork(AmmoniteWork work, void* userPtr) {
        ammonite::utils::thread::internal::submitWork(work, userPtr, nullptr);
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer and completion
         - If completion isn't a nullptr, it should be initialised with ATOMIC_FLAG_INIT
         - userPtr may be a nullptr
      */
      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion) {
        ammonite::utils::thread::internal::submitWork(work, userPtr, completion);
      }

      /*
       - Submit multiple jobs to the thread pool, with a user-provided buffer and completions
         - userBuffer should either be a nullptr, or an array of data to be split between jobs
           - Each job will receive a section according to (userBuffer + job index * stride)
           - stride should by the size of each section to give to a job, in bytes
         - group should either be a nullptr, or an AmmoniteGroup{jobCount}
         - jobCount specifies how many times submit the job
      */
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteGroup* group, unsigned int jobCount) {
        return ammonite::utils::thread::internal::submitMultiple(work, userBuffer,
          stride, group, jobCount);
      }

      //Wait for completion to be finished, completion can't be a nullptr
      void waitWorkComplete(AmmoniteCompletion* completion) {
        //Wait for completion to become true
        completion->wait(false);
      }

      //Wait for a group to be finished, group can't be a nullptr
      void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount) {
        //Wait for group to finish
        for (unsigned int i = 0; i < jobCount; i++) {
          group->acquire();
        }
      }

      //Block the thread pool from starting newer jobs, return after it takes effect
      void blockThreads() {
        ammonite::utils::thread::internal::blockThreads();
      }

      //Unblock the thread pool from starting newer jobs, return after it takes effect
      void unblockThreads() {
        ammonite::utils::thread::internal::unblockThreads();
      }

      //Wait until all work in the pool as of the call is finished
      void finishWork() {
        ammonite::utils::thread::internal::finishWork();
      }
    }
  }
}

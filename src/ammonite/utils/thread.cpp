#include <iostream>

#include "thread.hpp"

#include "debug.hpp"
#include "logging.hpp"

namespace ammonite {
  namespace utils {
    namespace thread {
      namespace {
        unsigned int poolUsers = 0;
      }

      //Return the number of hardware threads available
      unsigned int getHardwareThreadCount() {
        return internal::getHardwareThreadCount();
      }

      /*
       - Return the number of threads in the pool
         - Returns 0 if the pool doesn't exist
      */
      unsigned int getThreadPoolSize() {
        return internal::getThreadPoolSize();
      }

      /*
       - Create or join a thread pool, without initialising the renderer
         - The engine will share the thread pool if it's not destroyed before the
           renderer is initialised
       - destroyThreadPool() is still safe to call after renderer initialisation
       - Returns false if no thread pool exists or was created, otherwise true
      */
      bool createThreadPool(unsigned int threadCount) {
        bool exists = true;
        if (poolUsers == 0) {
          exists = internal::createThreadPool(threadCount);
        }

        poolUsers++;
        return exists;
      }

      /*
       - Destroy or exit the current thread pool
       - Must be called once per creation / connection
       - Safe to be called after renderer initialisation
       - If jobs in the queue may more submit work, they must be completed before calling this
      */
      void destroyThreadPool() {
        if (poolUsers == 0) {
          ammonite::utils::warning << "Attempted to destroy a thread pool before creation, ignoring" \
                                   << std::endl;
          return;
        }

        poolUsers--;
        if (poolUsers == 0) {
          internal::destroyThreadPool();
        } else {
          ammoniteInternalDebug << "Skipping thread pool destruction, " \
                                << poolUsers << " users remain" << std::endl;
        }
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer
         - userPtr may be a nullptr
       - createThreadPool() must be called before using this
       - Do not submit jobs that block conditionally on other jobs
      */
      void submitWork(AmmoniteWork work, void* userPtr) {
        internal::submitWork(work, userPtr, nullptr);
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer and group
         - group should either be a nullptr, or an AmmoniteGroup{0}
           - A group can be used between multiple calls, but waiting on it will block
             until all work in the group is done
         - userPtr may be a nullptr
       - createThreadPool() must be called before using this
       - Do not submit jobs that block conditionally on other jobs
      */
      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group) {
        internal::submitWork(work, userPtr, group);
      }

      /*
       - Submit multiple jobs to the thread pool, with a user-provided buffer and group
         - userBuffer should either be a nullptr, or an array of data to be split between jobs
           - Each job will receive a section according to (userBuffer + job index * stride)
           - stride should be the size of each section to give to a job, in bytes
         - group should either be a nullptr, or an AmmoniteGroup{0}
         - submitGroup should either be a nullptr, or an AmmoniteGroup{0}
         - jobCount specifies how many times to submit the job
       - Jobs are submitted asynchronously, waiting on submitGroup for 1 job can be used
         to wait for the submit to be complete
         - Waiting on submitGroup or group must be done before destroying the thread pool
         - This may be a while, use submitMultipleSync() instead of immediately waiting
       - createThreadPool() must be called before using this
       - Do not submit jobs that block conditionally on other jobs
      */
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteGroup* group, unsigned int jobCount,
                          AmmoniteGroup* submitGroup) {
        //Set stride to 0 when no data is passed
        if (userBuffer == nullptr) {
          stride = 0;
        }

        internal::submitMultiple(work, userBuffer, stride, group, jobCount, submitGroup);
      }

      //Synchronous version of submitMultiple()
      void submitMultipleSync(AmmoniteWork work, void* userBuffer, int stride,
                              AmmoniteGroup* group, unsigned int jobCount) {
        //Set stride to 0 when no data is passed
        if (userBuffer == nullptr) {
          stride = 0;
        }

        internal::submitMultipleSync(work, userBuffer, stride, group, jobCount);
      }

      /*
       - Wait for a group to be finished
       - jobCount determines how many jobs to wait for
         - If less than jobCount jobs have been given the group, this will block forever
         - It doesn't matter if the jobs have already finished
      */
      void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount) {
        if (group != nullptr) {
          internal::waitGroupComplete(group, jobCount);
        } else {
          ammoniteInternalDebug << "Group is a nullptr, skipping wait" << std::endl;
        }
      }

      /*
       - Check if at least one item of a group has finished
         - May spuriously fail, returning false when work had finished
       - Acts like synchronisation if successful, decreasing the group's counter
         - A second call to a group with 1 complete work item would return false
         - Using waitGroupComplete() at this point would block
       - If unsuccessful, nothing in the group is modified
      */
      bool isSingleWorkComplete(AmmoniteGroup* group) {
        if (group != nullptr) {
          return internal::isSingleWorkComplete(group);
        }

        ammoniteInternalDebug << "Group is a nullptr, skipping check" << std::endl;
        return false;
      }

      /*
       - Return the number of unfinished jobs in a group
       - Successive calls should use the remaining jobs returned as the job count
         - Subtract any synchronised / successfully queried jobs from this too
       - Remaining work may be overestimated, but never underestimated
       - Acts like synchronisation if successful, decreasing the group's counter
       - If unsuccessful, nothing in the group is modified
      */
      unsigned int getRemainingWork(AmmoniteGroup* group, unsigned int jobCount) {
        if (group != nullptr) {
          return internal::getRemainingWork(group, jobCount);
        }

        ammoniteInternalDebug << "Group is a nullptr, skipping query" << std::endl;
        return jobCount;
      }

      /*
       - Block the pool from starting new jobs
       - Returns once all threads are blocked
       - This isn't thread safe, and must never be called from a job
      */
      void blockThreads() {
        if (poolUsers != 0) {
          internal::blockThreads();
        }
      }

      /*
       - Allow the pool to start new jobs again
       - Returns once threads are have woken up
       - This isn't thread safe, and must never be called from a job
      */
      void unblockThreads() {
        if (poolUsers != 0) {
          internal::unblockThreads();
        }
      }

      /*
       - Wait until all work in the pool as of the call is finished
         - If a job submits more work while executing, the extra work won't be waited for
         - This includes submitMultiple(), which submits a job to submit the actual jobs
       - This isn't thread safe, and must never be called from a job
      */
      void finishWork() {
        if (poolUsers != 0) {
          internal::finishWork();
        }
      }
    }
  }
}

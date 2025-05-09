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
        return ammonite::utils::thread::internal::getHardwareThreadCount();
      }

      /*
       - Return the number of threads in the pool
         - Returns 0 if the pool doesn't exist
      */
      unsigned int getThreadPoolSize() {
        return ammonite::utils::thread::internal::getThreadPoolSize();
      }

      /*
       - Create or join a thread pool, without initialising the renderer
       - The engine will share the thread pool if it's not destroyed before the
         renderer is initialised
       - destroyThreadPool() is still safe to call after renderer initialisation
       - Returns false if no thread pool exists or was created
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
      */
      void destroyThreadPool() {
        if (poolUsers == 0) {
          ammonite::utils::warning << "Attempted to destroy a thread pool before creation, ignoring" \
                                   << std::endl;
          return;
        }

        poolUsers--;
        if (poolUsers == 0) {
          ammonite::utils::thread::internal::destroyThreadPool();
        } else {
          ammoniteInternalDebug << "Skipping thread pool destruction, " \
                                << poolUsers << " users remain" << std::endl;
        }
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer
         - userPtr may be a nullptr
      */
      void submitWork(AmmoniteWork work, void* userPtr) {
        ammonite::utils::thread::internal::submitWork(work, userPtr, nullptr);
      }

      /*
       - Submit a job to the thread pool, with a user-provided pointer and group
         - group should either be a nullptr, or an AmmoniteGroup{0}
           - A group can be used between multiple calls, but waiting on it will block
             until all work in the group is done
         - userPtr may be a nullptr
      */
      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group) {
        ammonite::utils::thread::internal::submitWork(work, userPtr, group);
      }

      /*
       - Submit multiple jobs to the thread pool, with a user-provided buffer and group
         - userBuffer should either be a nullptr, or an array of data to be split between jobs
           - Each job will receive a section according to (userBuffer + job index * stride)
           - stride should by the size of each section to give to a job, in bytes
         - group should either be a nullptr, or an AmmoniteGroup{0}
         - jobCount specifies how many times submit the job
      */
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteGroup* group, unsigned int jobCount) {
        ammonite::utils::thread::internal::submitMultiple(work, userBuffer, stride,
          group, jobCount);
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

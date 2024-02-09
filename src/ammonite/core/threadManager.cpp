#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>

#include "../types.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

#define MAX_EXTRA_THREADS 512

namespace ammonite {
  namespace thread {
    namespace internal {
      namespace {
        unsigned int extraThreadCount = 0;
        struct ThreadInfo {
          std::thread thread;
          std::mutex threadLock;
        };
        ThreadInfo* threadPool;
        std::condition_variable wakePool;
        bool stayAlive = false;

        struct WorkItem {
          AmmoniteWork work;
          void* userPtr;
          std::atomic_flag* completion;
        };

        //Write 'true' to unblockThreadsTrigger to release blocked threads
        std::atomic_flag unblockThreadsTrigger;
        //threadsUnblockedFlag is set to 'true' when all blocked threads are released
        //threadsUnblockedFlag is set to 'false' when all blocked threads are blocked
        //If transitioning between the two, it'll remain at its old value until complete
        std::atomic_flag threadsUnblockedFlag;
        std::atomic<unsigned int> blockedThreadCount;
        //0 when an unblock starts, 1 when a block starts
        //Any other value means the system is broken
        std::atomic<int> blockBalance;

        std::queue<WorkItem> workQueue;
        std::mutex workQueueMutex;
        std::atomic<int> jobCount{};
      }

      namespace {
        static void initWorker(ThreadInfo* threadInfo) {
          std::unique_lock<std::mutex> lock(threadInfo->threadLock);
          while (stayAlive) {
            workQueueMutex.lock();
            if (!workQueue.empty()) {
              //Fetch the work
              WorkItem workItem = workQueue.front();
              workQueue.pop();
              workQueueMutex.unlock();

              //Execute the work
              jobCount--;
              workItem.work(workItem.userPtr);

              //Set the completion, if given
              if (workItem.completion != nullptr) {
                workItem.completion->test_and_set();
                workItem.completion->notify_all();
              }
            } else {
              workQueueMutex.unlock();
              //Sleep until told to wake up
              wakePool.wait(lock, []{ return (!stayAlive or (jobCount > 0)); });
            }
          }
        }

        //Wait until unblockThreadsTrigger becomes true
        static void blocker(void*) {
          blockedThreadCount++;
          if (blockedThreadCount == extraThreadCount) {
            threadsUnblockedFlag.clear();
            threadsUnblockedFlag.notify_all();
          }

          unblockThreadsTrigger.wait(false);

          blockedThreadCount--;
          if (blockedThreadCount == 0) {
            threadsUnblockedFlag.test_and_set();
            threadsUnblockedFlag.notify_all();
          }
        }
      }

      unsigned int getHardwareThreadCount() {
        return std::thread::hardware_concurrency();
      }

      unsigned int getThreadPoolSize() {
        return extraThreadCount;
      }

      void submitWork(AmmoniteWork work, void* userPtr, std::atomic_flag* completion) {
        //Initialise the completion atomic
        if (completion != nullptr) {
          completion->clear();
        }

        //Safely add work to the queue
        workQueueMutex.lock();
        workQueue.push({work, userPtr, completion});
        workQueueMutex.unlock();

        //Increase job count, wake a sleeping thread
        jobCount++;
        wakePool.notify_one();
      }

      //Specialised variants to submit multiple jobs
      void submitMultiple(AmmoniteWork work, int newJobs) {
        workQueueMutex.lock();
        for (int i = 0; i < newJobs; i++) {
          workQueue.push({work, nullptr, nullptr});
          jobCount++;
          wakePool.notify_one();
        }
        workQueueMutex.unlock();
      }

      void submitMultipleUser(AmmoniteWork work, void** userPtrs, int newJobs) {
        workQueueMutex.lock();
        for (int i = 0; i < newJobs; i++) {
          workQueue.push({work, userPtrs[i], nullptr});
          jobCount++;
          wakePool.notify_one();
        }
        workQueueMutex.unlock();
      }

      void submitMultipleComp(AmmoniteWork work, std::atomic_flag* completions, int newJobs) {
        workQueueMutex.lock();
        for (int i = 0; i < newJobs; i++) {
          workQueue.push({work, nullptr, completions + i});
          jobCount++;
          wakePool.notify_one();
        }
        workQueueMutex.unlock();
      }

      void submitMultipleUserComp(AmmoniteWork work, void** userPtrs,
                                  std::atomic_flag* completions, int newJobs) {
        workQueueMutex.lock();
        for (int i = 0; i < newJobs; i++) {
          workQueue.push({work, userPtrs[i], completions + i});
          jobCount++;
          wakePool.notify_one();
        }
        workQueueMutex.unlock();
      }

      //Create thread pool, existing work will begin executing
      int createThreadPool(unsigned int extraThreads) {
        //Exit if thread pool already exists
        if (extraThreadCount != 0) {
          return -1;
        }

        //Default to creating a worker thread for every hardware thread
        if (extraThreads == 0) {
          extraThreads = getHardwareThreadCount();
        }

        //Cap at configured thread limit, allocate memory for pool
        extraThreads = (extraThreads > MAX_EXTRA_THREADS) ? MAX_EXTRA_THREADS : extraThreads;
        threadPool = new ThreadInfo[extraThreads];
        if (threadPool == nullptr) {
          return -1;
        }

        //Create the threads for the pool
        stayAlive = true;
        for (unsigned int i = 0; i < extraThreads; i++) {
          threadPool[i].thread = std::thread(initWorker, &threadPool[i]);
        }

        unblockThreadsTrigger.test_and_set();
        threadsUnblockedFlag.test_and_set();
        blockedThreadCount = 0;
        blockBalance = 0;
        extraThreadCount = extraThreads;
        return 0;
      }

      //Jobs submitted at the same time may execute, but the threads will block after
      //Guarantees work submitted after won't begin yet
      void blockThreads(bool sync) {
        //Skip blocking if it's already blocked / going to block
        if (blockBalance > 0) {
          return;
        }
        blockBalance++;

        //Submit a job for each thread that waits for the trigger
        unblockThreadsTrigger.clear();
        workQueueMutex.lock();
        for (unsigned int i = 0; i < extraThreadCount; i++) {
          workQueue.push({blocker, nullptr, nullptr});
        }
        workQueueMutex.unlock();

        //Add to job count and wake all threads
        jobCount += extraThreadCount;
        wakePool.notify_all();

        if (sync) {
          threadsUnblockedFlag.wait(true);
        }
      }

      void unblockThreads(bool sync) {
        //Only unblock if it's already blocked / blocking
        if (blockBalance == 0) {
          return;
        }
        blockBalance--;

        //Unblock threads and wake them up
        unblockThreadsTrigger.test_and_set();
        unblockThreadsTrigger.notify_all();

        if (sync) {
          threadsUnblockedFlag.wait(false);
        }
      }

#ifdef DEBUG
      bool debugCheckRemainingWork(bool verbose) {
        bool issuesFound = false;

        workQueueMutex.lock();
        if (workQueue.size() != 0) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Work queue not empty (" << workQueue.size() \
                                  << " jobs left)" << std::endl;
          }
        }

        if (workQueue.size() != (unsigned)jobCount) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Work queue size and job count don't match (" \
                                  << workQueue.size() << " vs " << jobCount << ")" << std::endl;
          }
        }

        if (jobCount < 0) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Job count is negative (" \
                                  << jobCount << ")" << std::endl;
          }
        }
        workQueueMutex.unlock();

        return issuesFound;
      }
#endif

      //Finish work already in the queue and kill the threads
      void destroyThreadPool() {
        //Finish existing work and block new work from starting
        unblockThreads(true);
        blockThreads(true);

        //Kill all threads when the're unblocked wake up
        stayAlive = false;

        //Unblock threads and wake them up
        unblockThreads(true);

        //Wait until all threads are done
        for (unsigned int i = 0; i < extraThreadCount; i++) {
          try {
            threadPool[i].thread.join();
          } catch (const std::system_error&) {
            ammonite::utils::warning << "Failed to join thread " << i \
                                     << " while destroying thread pool" << std::endl;
          }
        }

/* In debug mode, check that the queue is empty, and matches the job counter
 - If it doesn't, it's not technically a bug, but in practice it will be, as work
   that was expected to be finished wasn't
*/
#ifdef DEBUG
        debugCheckRemainingWork(true);
#endif

        //Reset remaining data
        delete[] threadPool;
        extraThreadCount = 0;
      }
    }
  }
}

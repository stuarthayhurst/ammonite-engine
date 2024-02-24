#include <atomic>
#include <cstring>
#include <queue>
#include <thread>

#include "../types.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

#define MAX_EXTRA_THREADS 512
//Max of 2,097,152 work items
#define QUEUE_SIZE (2 << 20)

namespace {
  struct WorkItem {
    AmmoniteWork work;
    void* userPtr;
    std::atomic_flag* completion;
  };

  class WorkQueue {
  private:
    WorkItem* workItems;
    bool* readyPtr;
    int queueSize;

    std::atomic<unsigned int> nextWrite = 0;
    unsigned int nextRead = 0;
    std::mutex readLock;

  public:
    WorkQueue(int size) {
      workItems = new WorkItem[size];
      readyPtr = new bool[size];
      std::memset(workItems, 0, size * sizeof(WorkItem));
      std::memset(readyPtr, 0, size * sizeof(bool));
      queueSize = size;

      if (((size & (size - 1)) != 0) && size != 0) {
        ammonite::utils::warning << "Queue size must be a non-zero power of 2, good luck" \
                                 << std::endl;
      }
    }

    ~WorkQueue() {
      delete [] workItems;
    }

    void pop(WorkItem* workPtr) {
      readLock.lock();
      int index = (nextRead) & (queueSize - 1);

      //Return the work item or an empty one
      if (readyPtr[index]) {
        nextRead++;
        readLock.unlock();
        readyPtr[index] = false;
        *workPtr = workItems[index];
      } else {
        readLock.unlock();
        *workPtr = {nullptr, nullptr, nullptr};
      }
    }

    void push(WorkItem* workPtr) {
      int index = (nextWrite++) & (queueSize - 1);
      workItems[index] = *workPtr;
      readyPtr[index] = true;

#ifdef DEBUG
      if (this->getSize() > QUEUE_SIZE) {
        ammonite::utils::warning << "Queue size exceeded, work items lost" \
                                 << std::endl;
      }
#endif
    }

    int getSize() {
      return nextWrite - nextRead;
    }
  };
}

namespace ammonite {
  namespace thread {
    namespace internal {
      namespace {
        unsigned int extraThreadCount = 0;
        struct ThreadInfo {
          std::thread thread;
        };
        ThreadInfo* threadPool;
        bool stayAlive = false;

        //Write 'true' to unblockThreadsTrigger to release blocked threads
        std::atomic_flag unblockThreadsTrigger;
        //threadsUnblockedFlag is set to 'true' when all blocked threads are released
        //threadsUnblockedFlag is set to 'false' when all blocked threads are blocked
        //If transitioning between the two, it'll remain at its old value until complete
        std::atomic_flag threadsUnblockedFlag;
        std::atomic<unsigned int> blockedThreadCount;
        //0 when an unblock starts, 1 when a block starts
        //Any other value means the system is broken
        std::atomic<int> blockBalance = 0;

        WorkQueue* workQueue;
        std::atomic<int> jobCount = 0;
      }

      namespace {
        static void initWorker() {
          while (stayAlive) {
            //Fetch the work
            WorkItem workItem;
            workQueue->pop(&workItem);

            //Execute the work or sleep
            if (workItem.work != nullptr) {
              jobCount--;
              workItem.work(workItem.userPtr);

              //Set the completion, if given
              if (workItem.completion != nullptr) {
                workItem.completion->test_and_set();
                workItem.completion->notify_all();
              }
            } else {
              //Sleep while 0 jobs remain
              jobCount.wait(0);
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

        //Add work to the queue
        WorkItem workItem = {work, userPtr, completion};
        workQueue->push(&workItem);

        //Increase job count, wake a sleeping thread
        jobCount++;
        jobCount.notify_one();
      }

      //Specialised variants to submit multiple jobs
      void submitMultiple(AmmoniteWork work, int newJobs) {
        WorkItem workItem = {work, nullptr, nullptr};
        for (int i = 0; i < newJobs; i++) {
          workQueue->push(&workItem);
          jobCount++;
          jobCount.notify_one();
        }
      }

      void submitMultipleUser(AmmoniteWork work, void** userPtrs, int newJobs) {
        for (int i = 0; i < newJobs; i++) {
        WorkItem workItem = {work, userPtrs[i], nullptr};
          workQueue->push(&workItem);
          jobCount++;
          jobCount.notify_one();
        }
      }

      void submitMultipleComp(AmmoniteWork work, std::atomic_flag* completions, int newJobs) {
        for (int i = 0; i < newJobs; i++) {
          WorkItem workItem = {work, nullptr, completions + i};
          workQueue->push(&workItem);
          jobCount++;
          jobCount.notify_one();
        }
      }

      void submitMultipleUserComp(AmmoniteWork work, void** userPtrs,
                                  std::atomic_flag* completions, int newJobs) {
        for (int i = 0; i < newJobs; i++) {
          WorkItem workItem = {work, userPtrs[i], completions + i};
          workQueue->push(&workItem);
          jobCount++;
          jobCount.notify_one();
        }
      }

      //Create thread pool, existing work will begin executing
      int createThreadPool(unsigned int extraThreads) {
        //Exit if thread pool already exists
        if (extraThreadCount != 0) {
          return -1;
        }

        //Create the queue
        workQueue = new WorkQueue(QUEUE_SIZE);

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
          threadPool[i].thread = std::thread(initWorker);
        }

        unblockThreadsTrigger.test_and_set();
        threadsUnblockedFlag.test_and_set();
        blockedThreadCount = 0;
        extraThreadCount = extraThreads;
        return 0;
      }

      //Jobs submitted at the same time may execute, but the threads will block after
      //Guarantees work submitted after won't begin yet
      //Unsafe to call from multiple threads due to blockBalance
      void blockThreads(bool sync) {
        //Skip blocking if it's already blocked / going to block
        if (blockBalance > 0) {
          return;
        }
        blockBalance++;

        //Submit a job for each thread that waits for the trigger
        unblockThreadsTrigger.clear();
        WorkItem blockerItem = {blocker, nullptr, nullptr};
        for (unsigned int i = 0; i < extraThreadCount; i++) {
          workQueue->push(&blockerItem);
        }

        //Add to job count and wake all threads
        jobCount += extraThreadCount;
        jobCount.notify_all();

        if (sync) {
          threadsUnblockedFlag.wait(true);
        }
      }

      //Unsafe to call from multiple threads due to blockBalance
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

      void finishWork() {
        //Unblock if blocked, block threads then wait for completion
        unblockThreads(true);
        blockThreads(true);
        unblockThreads(true);
      }

#ifdef DEBUG
      bool debugCheckRemainingWork(bool verbose) {
        bool issuesFound = false;

        if (workQueue->getSize() != 0) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Work queue not empty (" << workQueue->getSize() \
                                  << " jobs left)" << std::endl;
          }
        }

        if (workQueue->getSize() != jobCount) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Work queue size and job count don't match (" \
                                  << workQueue->getSize() << " vs " << jobCount << ")" << std::endl;
          }
        }

        if (jobCount < 0) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Job count is negative (" \
                                  << jobCount << ")" << std::endl;
          }
        }

        if (blockBalance != 0) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Blocking is unbalanced (" \
                                  << blockBalance << ")" << std::endl;
          }
        }

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
        delete workQueue;
        delete[] threadPool;
        extraThreadCount = 0;
      }
    }
  }
}

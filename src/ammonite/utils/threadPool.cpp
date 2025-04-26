#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <semaphore>
#include <system_error>
#include <thread>

#include "threadPool.hpp"

#include "logging.hpp"
#include "../types.hpp"

static constexpr unsigned int MAX_THREADS = 512;

namespace {
  struct WorkItem {
    AmmoniteWork work;
    void* userPtr;
    AmmoniteGroup* group;
  };

  //Implements a thread-safe queue to store and retrieve jobs from
  class WorkQueue {
  private:
    std::queue<WorkItem> queue;
    std::mutex queueLock;
    std::counting_semaphore<std::numeric_limits<int32_t>::max()> jobCount{0};

  public:
    void push(AmmoniteWork work, void* userPtr, AmmoniteGroup* group) {
      queueLock.lock();
      queue.push({work, userPtr, group});
      queueLock.unlock();
      jobCount.release();
    }


    void pushMultiple(AmmoniteWork work, void* userBuffer, int stride,
                      AmmoniteGroup* group, unsigned int count) {
      queueLock.lock();
      for (unsigned int i = 0; i < count; i++) {
        queue.push({work, (char*)userBuffer + ((std::size_t)(i) * stride), group});
      }

      queueLock.unlock();
      jobCount.release(count);
    }


    void pop(WorkItem* workItemPtr) {
      jobCount.acquire();
      queueLock.lock();
      *workItemPtr = queue.front();
      queue.pop();
      queueLock.unlock();
    }
  };
}

namespace ammonite {
  namespace utils {
    namespace thread {
      namespace internal {
        namespace {
          unsigned int poolThreadCount = 0;
          std::thread* threadPool;
          bool stayAlive = false;

          //Write true / false to threadBlockTrigger to block / unblock threads
          std::atomic<bool> threadBlockTrigger{false};
          std::atomic<unsigned int> blockedThreadCount{0};
          std::atomic<bool> threadsBlocked{false};

          WorkQueue* workQueues;
          unsigned int queueLaneCount = 0;
          unsigned int queueLaneMask = 0;
          std::atomic<uintmax_t> nextJobRead = 0;
          std::atomic<uintmax_t> nextJobWrite = 0;
        }

        namespace {
          void initWorker() {
            while (stayAlive) {
              //Fetch the work
              WorkItem workItem = {nullptr, nullptr, nullptr};

              //Wait for a job to be available
              const uintmax_t targetQueue = (nextJobRead++) & queueLaneMask;
              workQueues[targetQueue].pop(&workItem);

              //Execute the work
              workItem.work(workItem.userPtr);

              //Update the group semaphore, if given
              if (workItem.group != nullptr) {
                workItem.group->release();
              }
            }
          }

          void blocker(void*) {
            if (++blockedThreadCount == poolThreadCount) {
              threadsBlocked = true;
              threadsBlocked.notify_all();
            }

            threadBlockTrigger.wait(true);

            if (--blockedThreadCount == 0) {
              threadsBlocked = false;
              threadsBlocked.notify_all();
            }
          }
        }

        unsigned int getHardwareThreadCount() {
          return std::thread::hardware_concurrency();
        }

        unsigned int getThreadPoolSize() {
          return poolThreadCount;
        }

        void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group) {
          //Add work to the queue
          const uintmax_t targetQueue = (nextJobWrite++) & queueLaneMask;
          workQueues[targetQueue].push(work, userPtr, group);
        }

        /*
         - Submit multiple jobs in a batch, with no ordering guarantees
         - Even carefully crafted blocking jobs that depend on other jobs will fail
           - For the special 'blocker', use submitWork instead
        */
        void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                            AmmoniteGroup* group, unsigned int newJobs) {
          //Every queue gets at least baseBatchSize jobs
          const unsigned int baseBatchSize = newJobs / queueLaneCount;

          /*
           - Add the base amount of work to each queue without touching the atomic index
           - This only works because every queue is being given an equal amount of work
             - This doesn't work if the job will block until all jobs are executing
          */
          if (baseBatchSize > 0) {
            for (unsigned int i = 0; i < queueLaneCount; i++) {
              workQueues[i].pushMultiple(work, userBuffer, stride, group, baseBatchSize);
              userBuffer = (char*)userBuffer + (std::size_t(baseBatchSize) * stride);
            }
          }

          //Add the remaining work
          const unsigned int remainingJobs = newJobs - (baseBatchSize * queueLaneCount);
          for (unsigned int i = 0; i < remainingJobs; i++) {
            const uintmax_t targetQueue = (nextJobWrite++) & queueLaneMask;
            workQueues[targetQueue].push(work, (char*)userBuffer + (std::size_t(i) * stride), group);
          }
        }

        //Create thread pool of the requested size, if one doesn't already exist
        bool createThreadPool(unsigned int threadCount) {
          //Exit if thread pool already exists
          if (poolThreadCount != 0) {
            return false;
          }

          //Default to creating a worker thread for every hardware thread
          if (threadCount == 0) {
            threadCount = getHardwareThreadCount();
          }

          //Cap at configured thread limit, allocate memory for pool
          threadCount = (threadCount > MAX_THREADS) ? MAX_THREADS : threadCount;
          threadPool = new std::thread[threadCount];
          if (threadPool == nullptr) {
            return false;
          }

          //Round thread count up to nearest power of 2 to decide queue count
          queueLaneCount = std::bit_ceil(threadCount);
          queueLaneMask = queueLaneCount - 1;

          //Create the queues
          workQueues = new WorkQueue[queueLaneCount];
          poolThreadCount = threadCount;

          //Create the queues threads for the pool
          stayAlive = true;
          for (unsigned int i = 0; i < threadCount; i++) {
            threadPool[i] = std::thread(initWorker);
          }

          return true;
        }

        /*
         - Prevent the threads from executing newly submitted jobs
         - Work submitted after the call returns is guaranteed to be blocked
         - Return when the block takes effect
        */
        void blockThreads() {
          if (!threadsBlocked) {
            threadBlockTrigger = true;
            for (unsigned int i = 0; i < poolThreadCount; i++) {
              //submitMultiple can't be used as it doesn't assign atomically from the current position
              submitWork(blocker, nullptr, nullptr);
            }

            threadsBlocked.wait(false);
          }
        }

        /*
         - Allow the threads to execute queued jobs
         - Return when all threads are unblocked
        */
        void unblockThreads() {
          if (threadsBlocked) {
            threadBlockTrigger = false;
            threadBlockTrigger.notify_all();

            threadsBlocked.wait(true);
          }
        }

        //Finish all work in the queue as of calling, return when done
        void finishWork() {
          //Unblock if blocked, block threads then wait for completion
          unblockThreads();
          blockThreads();
          unblockThreads();
        }

        //Finish work already in the queue and kill the threads
        void destroyThreadPool() {
          //Finish existing work and block new work from starting
          unblockThreads();
          blockThreads();

          //Kill all threads after they're unblocked and wake up
          stayAlive = false;

          //Unblock threads and wake them up
          unblockThreads();

          //Wait until all threads are done
          for (unsigned int i = 0; i < poolThreadCount; i++) {
            try {
              threadPool[i].join();
            } catch (const std::system_error&) {
              ammonite::utils::warning << "Failed to join thread " << i \
                                       << " while destroying thread pool" << std::endl;
            }
          }

          //Reset remaining data
          delete [] workQueues;
          delete [] threadPool;
          queueLaneCount = 0;
          queueLaneMask = 0;
          poolThreadCount = 0;
        }
      }
    }
  }
}

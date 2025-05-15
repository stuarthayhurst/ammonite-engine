#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <system_error>
#include <thread>

#include "thread.hpp"

#include "debug.hpp"
#include "logging.hpp"

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

  public:
    void push(AmmoniteWork work, void* userPtr, AmmoniteGroup* group) {
      queueLock.lock();
      queue.push({work, userPtr, group});
      queueLock.unlock();
    }

    void pushMultiple(AmmoniteWork work, void* userBuffer, int stride,
                      AmmoniteGroup* group, unsigned int count) {
      //Add multiple jobs in a single pass
      queueLock.lock();
      for (std::size_t i = 0; i < count; i++) {
        queue.push({work, (char*)userBuffer + (i * stride), group});
      }
      queueLock.unlock();
    }

    void pop(WorkItem* workItemPtr) {
      queueLock.lock();
      if (!queue.empty()) {
        *workItemPtr = queue.front();
        queue.pop();
      } else {
        workItemPtr->work = nullptr;
      }
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

          WorkQueue* workQueue;
          std::atomic<uintmax_t> jobCount = 0;
        }

        namespace {
          void initWorker() {
            while (stayAlive) {
              //Fetch the work
              WorkItem workItem;
              workQueue->pop(&workItem);

              //Execute the work or sleep
              if (workItem.work != nullptr) {
                jobCount--;
                workItem.work(workItem.userPtr);

                //Update the group semaphore, if given
                if (workItem.group != nullptr) {
                  workItem.group->release();
                }
              } else {
                //Sleep while 0 jobs remain
                jobCount.wait(0);
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
          workQueue->push(work, userPtr, group);

          //Increase job count, wake a sleeping thread
          jobCount++;
          jobCount.notify_one();
        }

        //Submit multiple jobs without locking multiple times
        void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                            AmmoniteGroup* group, unsigned int newJobs) {
          workQueue->pushMultiple(work, userBuffer, stride, group, newJobs);
          jobCount += newJobs;
          jobCount.notify_all();
        }

        //Create thread pool, existing work will begin executing
        bool createThreadPool(unsigned int threadCount) {
          //Exit if thread pool already exists
          if (poolThreadCount != 0) {
            return false;
          }

          //Create the queue
          workQueue = new WorkQueue();

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

          //Create the threads for the pool
          stayAlive = true;
          for (unsigned int i = 0; i < threadCount; i++) {
            threadPool[i] = std::thread(initWorker);
          }

          poolThreadCount = threadCount;
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
            submitMultiple(blocker, nullptr, 0, nullptr, poolThreadCount);

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
          ammoniteInternalDebug << "Destroying thread pool" << std::endl;

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
          delete workQueue;
          delete [] threadPool;
          poolThreadCount = 0;
        }
      }
    }
  }
}

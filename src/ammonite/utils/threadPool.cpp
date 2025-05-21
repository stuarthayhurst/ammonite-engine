#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <system_error>
#include <thread>

#include <unistd.h>

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

          //Barrier to synchronise all threads
          std::barrier<>* threadSyncBarrier;

          //Trigger, flag and barrier to block all threads, then communicate completion
          std::atomic<bool> threadBlockTrigger = false;
          std::atomic<bool> threadsBlocked = false;
          std::barrier<void (*)()>* threadBlockBarrier;

          //The work queue doesn't need to be heap allocated, but seems to perform better when it is
          WorkQueue* workQueue;
          std::atomic<uintmax_t> jobCount = 0;
        }

        namespace {
          void runWorker() {
            //Ask the system for the thread ID, since it's more useful for debugging
            ammoniteInternalDebug << "Started worker thread (ID " << gettid() \
                                  << ")" << std::endl;

            while (stayAlive) {
              //Fetch the work
              WorkItem workItem;
              workQueue->pop(&workItem);

              /*
               - Execute the work or sleep
               - workItem.work may be nullptr if none was available
              */
              if (workItem.work != nullptr) {
                jobCount--;
                workItem.work(workItem.userPtr);

                //Update the group semaphore, if given
                if (workItem.group != nullptr) {
                  workItem.group->release();
                }
              }

              //Sleep while no jobs remain
              jobCount.wait(0);

              //Block the thread when instructed to, wait to release it
              if (threadBlockTrigger) {
                threadBlockBarrier->arrive_and_wait();
                threadBlockTrigger.wait(true);
              }
            }
          }

          /*
           - Callback for when threads are blocked
           - Mark threads as blocked
          */
          void threadsBlockedCallback() {
            threadsBlocked = true;
            threadsBlocked.notify_all();
          }

          //Simple job to synchronise threads
          void finishSyncJob(void*) {
            threadSyncBarrier->arrive_and_wait();
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
          ammoniteInternalDebug << "Creating thread pool with " << threadCount \
                                << " thread(s)" << std::endl;
          threadPool = new std::thread[threadCount];
          if (threadPool == nullptr) {
            delete workQueue;
            return false;
          }

          //Prepare thread finish barrier
          threadSyncBarrier = new std::barrier{threadCount};

          //Prepare thread block syncs
          threadBlockBarrier = new std::barrier{threadCount, threadsBlockedCallback};
          threadsBlocked = false;
          threadBlockTrigger = false;

          //Create the threads for the pool
          stayAlive = true;
          for (unsigned int i = 0; i < threadCount; i++) {
            threadPool[i] = std::thread(runWorker);
          }

          poolThreadCount = threadCount;
          return true;
        }

        /*
         - Instruct threads to block after their current job
         - Create a fake job for each thread in case they're asleep
         - Return once the threads are all blocked
        */
        void blockThreads() {
          if (!threadsBlocked) {
            threadBlockTrigger = true;

            //Threads need to be woken up, in case they're waiting for work
            jobCount += poolThreadCount;
            jobCount.notify_all();

            threadsBlocked.wait(false);
          }
        }

        /*
         - Instruct threads to resume execution, clean up fake jobs
         - Return as soon as possible, threads may still be asleep
        */
        void unblockThreads() {
          if (threadsBlocked) {
            //Correct the job count
            jobCount -= poolThreadCount;
            jobCount.notify_all();

            threadBlockTrigger = false;
            threadBlockTrigger.notify_all();

            threadsBlocked = false;
          }
        }

        /*
         - Complete all work already queued
         - Return when the work has finished
        */
        void finishWork() {
          AmmoniteGroup group{0};
          internal::submitMultiple(finishSyncJob, nullptr, 0, &group, poolThreadCount);

          waitGroupComplete(&group, poolThreadCount);
        }

        //Finish work already in the queue and kill the threads
        void destroyThreadPool() {
          ammoniteInternalDebug << "Destroying thread pool" << std::endl;

          if (threadsBlocked) {
            ammonite::utils::warning << "Attempting to destroy thread pool while blocked" \
                                     << std::endl;
          }

          //Finish existing work
          internal::finishWork();

          //Block threads, instruct them to die then unblock
          internal::blockThreads();
          stayAlive = false;
          internal::unblockThreads();

          //Wait until all threads are done
          for (unsigned int i = 0; i < poolThreadCount; i++) {
            try {
              threadPool[i].join();
            } catch (const std::system_error&) {
              ammonite::utils::warning << "Failed to join thread " << i \
                                       << " while destroying thread pool" << std::endl;
            }
          }

          if (jobCount != 0) {
            ammonite::utils::warning << "Thread pool destroyed with " << jobCount \
                                     << " jobs remaining" << std::endl;
          }

          //Reset remaining data
          delete workQueue;
          delete [] threadPool;
          delete threadSyncBarrier;
          delete threadBlockBarrier;
          poolThreadCount = 0;
          jobCount = 0;
        }
      }
    }
  }
}

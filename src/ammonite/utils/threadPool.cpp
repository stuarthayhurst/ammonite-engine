#include <atomic>
#include <barrier>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <latch>
#include <limits>
#include <mutex>
#include <queue>
#include <semaphore>
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

      //Add multiple jobs in a single pass
      for (std::size_t i = 0; i < count; i++) {
        queue.push({work, (char*)userBuffer + (i * stride), group});
      }

      queueLock.unlock();
      jobCount.release(count);
    }

    void pop(WorkItem* workItemPtr) {
      /*
       - TODO: libstdc++ has a deadlock in the implementation of counting_semaphore
         - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104928
       - The deadlock is extremely rare, so the performance impact of this workaround
         shouldn't be too bad
       - Once this is fixed, just use 'jobCount.acquire();' instead
      */
      while(!jobCount.try_acquire_for(std::chrono::milliseconds(1))) {}
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
          std::atomic<bool> stayAlive = false;

          //Barrier to synchronise all threads
          std::barrier<>* threadSyncBarrier;

          //Trigger, flag and barrier to block all threads, then communicate completion
          std::atomic<bool> threadBlockTrigger = false;
          bool threadsBlocked = false;
          std::barrier<>* threadBlockBarrier;
          std::latch* threadUnblockLatch;

          WorkQueue* workQueues;
          unsigned int queueLaneCount = 0;
          unsigned int laneAssignMask = 0;
          std::atomic<uintmax_t> nextJobRead = 0;
          std::atomic<uintmax_t> nextJobWrite = 0;
        }

        namespace {
          void runWorker() {
            //Ask the system for the thread ID, since it's more useful for debugging
            ammoniteInternalDebug << "Started worker thread (ID " << gettid() \
                                  << ")" << std::endl;

            WorkItem workItem = {nullptr, nullptr, nullptr};
            while (stayAlive) {
              //Wait for a job to be available, then return it
              const uintmax_t targetQueue = (nextJobRead++) & laneAssignMask;
              workQueues[targetQueue].pop(&workItem);

              //Block the thread when instructed to, wait to release it
              if (threadBlockTrigger) {
                threadBlockBarrier->arrive_and_wait();
                threadBlockTrigger.wait(true);

                //Mark thread as unblocked as it resumes
                threadUnblockLatch->count_down();
              }

              /*
               - Execute the work or sleep
               - workItem.work may be nullptr to wake the threads up
              */
              if (workItem.work != nullptr) {
                workItem.work(workItem.userPtr);

                //Update the group semaphore, if given
                if (workItem.group != nullptr) {
                  workItem.group->release();
                }
              }
            }
          }

          void wakeThreads() {
            for (unsigned int i = 0; i < poolThreadCount; i++) {
              internal::submitWork(nullptr, nullptr, nullptr);
            }
          }

          //Simple job to synchronise threads
          void finishSyncJob(void*) {
            threadSyncBarrier->arrive_and_wait();
          }
        }

        //Helpers for asynchronous submitMultiple()
        namespace {
          struct SubmitData {
            AmmoniteWork work;
            void* userBuffer;
            AmmoniteGroup* group;
            int stride;
            unsigned int jobCount;
          };

          void submitMultipleJob(void* rawSubmitData) {
            SubmitData* submitData = (SubmitData*)rawSubmitData;

            //Every queue gets at least baseBatchSize jobs
            //TODO: Remove this once clang-tidy-21 is released
            //NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
            const unsigned int baseBatchSize = submitData->jobCount / queueLaneCount;

            //Add the base amount of work to each queue without touching the atomic index
            if (baseBatchSize > 0) {
              const std::size_t jobSize = std::size_t(baseBatchSize) * submitData->stride;
              for (unsigned int i = 0; i < queueLaneCount; i++) {
                workQueues[i].pushMultiple(submitData->work, submitData->userBuffer,
                                           submitData->stride, submitData->group, baseBatchSize);
                submitData->userBuffer = (char*)submitData->userBuffer + jobSize;
              }
            }

            //Add the remaining work
            const unsigned int remainingJobs = submitData->jobCount - (baseBatchSize * queueLaneCount);
            for (unsigned int i = 0; i < remainingJobs; i++) {
              const uintmax_t targetQueue = (nextJobWrite++) & laneAssignMask;
              workQueues[targetQueue].push(submitData->work, (char*)submitData->userBuffer,
                                           submitData->group);
              submitData->userBuffer = (char*)submitData->userBuffer + submitData->stride;
            }

            delete submitData;
          }
        }

        unsigned int getHardwareThreadCount() {
          return std::thread::hardware_concurrency();
        }

        unsigned int getThreadPoolSize() {
          return poolThreadCount;
        }

        void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group) {
          //Add work to the next queue
          const uintmax_t targetQueue = (nextJobWrite++) & laneAssignMask;
          workQueues[targetQueue].push(work, userPtr, group);
        }

        /*
         - Submit a job that submits the actual work when executed
         - Submitting the actual jobs asynchronously returns faster, allowing
           the overhead to be mitigated by useful work
        */
        void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                            AmmoniteGroup* group, unsigned int newJobs,
                            AmmoniteGroup* submitGroup) {
          SubmitData* dataPtr = new SubmitData{work, userBuffer, group, stride, newJobs};
          internal::submitWork(submitMultipleJob, dataPtr, submitGroup);
        }

        //Synchronous version of submitMultiple()
        void submitMultipleSync(AmmoniteWork work, void* userBuffer, int stride,
                                AmmoniteGroup* group, unsigned int newJobs) {
          //Pack the data into the expected format and just execute the job immediately
          SubmitData* dataPtr = new SubmitData{work, userBuffer, group, stride, newJobs};
          submitMultipleJob(dataPtr);
        }

        //Wait for jobCount jobs in group to finish
        void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount) {
          for (unsigned int i = 0; i < jobCount; i++) {
            //TODO: Same issue as jobCount, see WorkQueue's implementation above
            while(!group->try_acquire_for(std::chrono::milliseconds(1))) {}
          }
        }

        //Create a thread pool of the requested size, if one doesn't already exist
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
          ammoniteInternalDebug << "Creating thread pool with " << threadCount \
                                << " thread(s)" << std::endl;
          threadPool = new std::thread[threadCount];
          if (threadPool == nullptr) {
            return false;
          }

          //Round thread count up to nearest power of 2 and double it to decide lane count
          queueLaneCount = std::bit_ceil(threadCount) * 2;
          laneAssignMask = queueLaneCount - 1;

          //Create the queues
          nextJobRead = 0;
          nextJobWrite = 0;
          workQueues = new WorkQueue[queueLaneCount];

          //Prepare thread finish barrier
          threadSyncBarrier = new std::barrier{threadCount};

          //Prepare thread block syncs
          threadBlockBarrier = new std::barrier{threadCount + 1};
          threadsBlocked = false;
          threadBlockTrigger = false;

          //Create the threads for the pool
          stayAlive = true;
          poolThreadCount = threadCount;
          for (unsigned int i = 0; i < poolThreadCount; i++) {
            threadPool[i] = std::thread(runWorker);
          }

          return true;
        }

        /*
         - Instruct threads to block after their current job
         - Create a fake job for each thread in case they're asleep
         - Return once the threads are all blocked
        */
        void blockThreads() {
          if (!threadsBlocked) {
            //Instruct thread to block
            threadBlockTrigger = true;

            //Threads need to be woken up, in case they're waiting for work
            wakeThreads();

            //Wait for threads to block
            threadBlockBarrier->arrive_and_wait();
            threadsBlocked = true;
          }
        }

        /*
         - Instruct threads to resume execution
         - Return as soon as all threads have woken up
        */
        void unblockThreads() {
          if (threadsBlocked) {
            //Prepare a latch for synchronising unblocking
            threadUnblockLatch = new std::latch{poolThreadCount + 1};

            //Instruct threads to unblock
            threadBlockTrigger = false;
            threadBlockTrigger.notify_all();

            //Wait for all thread to unblock, then clean up and return
            threadUnblockLatch->arrive_and_wait();
            threadsBlocked = false;
            delete threadUnblockLatch;
          }
        }

        /*
         - Complete all work already queued
         - Return when the work has finished
        */
        void finishWork() {
          AmmoniteGroup group{0};
          for (unsigned int i = 0; i < poolThreadCount; i++) {
            internal::submitWork(finishSyncJob, nullptr, &group);
          }

          internal::waitGroupComplete(&group, poolThreadCount);
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

          //Reset remaining data
          delete [] workQueues;
          delete [] threadPool;
          delete threadSyncBarrier;
          delete threadBlockBarrier;
          poolThreadCount = 0;
          queueLaneCount = 0;
          laneAssignMask = 0;
          nextJobRead = 0;
          nextJobWrite = 0;
        }
      }
    }
  }
}

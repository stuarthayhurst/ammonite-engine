#include <atomic>
#include <cstddef>
#include <semaphore>
#include <thread>

#include "../types.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"

#define MAX_THREADS 512

namespace {
  struct WorkItem {
    AmmoniteWork work;
    void* userPtr;
    AmmoniteCompletion* completion;
  };

  struct Node {
    WorkItem workItem;
    Node* nextNode;
  };

  class WorkQueue {
  private:
    std::binary_semaphore readSemaphore{1};
    Node* nextPopped;
    std::atomic<Node*> nextPushed;

  public:
    WorkQueue() {
      //Start with an empty queue, 1 'old' node
      nextPushed = new Node{{nullptr, nullptr, nullptr}, nullptr};
      nextPopped = nextPushed;
    }

    ~WorkQueue() {
      //Clear out any remaining nodes
      WorkItem workItem;
      do {
        this->pop(&workItem);
      } while (workItem.work != nullptr);

      //Clear up next free node
      delete nextPopped;
    }

    void push(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion) {
      //Create a new empty node
      Node* newNode = new Node{{nullptr, nullptr, nullptr}, nullptr};

      //Atomically swap the next node with newNode, then fill in the old new node now it's free
      *(nextPushed.exchange(newNode)) = {{work, userPtr, completion}, newNode};
    }

    void pushMultiple(AmmoniteWork work, void* userBuffer, int stride,
                      AmmoniteCompletion* completions, int count) {
      //Generate section of linked list to insert
      Node* newNode = new Node{{nullptr, nullptr, nullptr}, nullptr};
      Node sectionStart;
      Node* sectionPtr = &sectionStart;
      if (userBuffer == nullptr) {
        if (completions == nullptr) {
          for (int i = 0; i < count; i++) {
            sectionPtr->nextNode = new Node{{work, nullptr, nullptr}, nullptr};
            sectionPtr = sectionPtr->nextNode;
          }
        } else {
          for (int i = 0; i < count; i++) {
            sectionPtr->nextNode = new Node{{work, nullptr, completions + i}, nullptr};
            sectionPtr = sectionPtr->nextNode;
          }
        }
      } else {
        if (completions == nullptr) {
          for (int i = 0; i < count; i++) {
            sectionPtr->nextNode = new Node{{
              work, (void*)((char*)userBuffer + (std::size_t)(i * stride)), nullptr}, nullptr};
            sectionPtr = sectionPtr->nextNode;
          }
        } else {
          for (int i = 0; i < count; i++) {
            sectionPtr->nextNode = new Node{{
              work, (void*)((char*)userBuffer + (std::size_t)(i * stride)), completions + i}, nullptr};
            sectionPtr = sectionPtr->nextNode;
          }
        }
      }

      //Insert the generated section atomically
      sectionPtr->nextNode = newNode;
      *(nextPushed.exchange(newNode)) = *sectionStart.nextNode;
      delete sectionStart.nextNode;
    }

    void pop(WorkItem* workItemPtr) {
      //Use the most recently popped node to find the next
      readSemaphore.acquire();

      //Copy the data and free the old node, otherwise return if we don't have a new node
      Node* currentNode = nextPopped;
      if (currentNode->nextNode != nullptr) {
        nextPopped = nextPopped->nextNode;
        readSemaphore.release();

        *workItemPtr = currentNode->workItem;
        delete currentNode;
      } else {
        readSemaphore.release();
        workItemPtr->work = nullptr;
      }
    }
  };
}

namespace ammonite {
  namespace thread {
    namespace internal {
      namespace {
        unsigned int poolThreadCount = 0;
        std::thread* threadPool;
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
          if (blockedThreadCount == poolThreadCount) {
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
        return poolThreadCount;
      }

      void submitWork(AmmoniteWork work, void* userPtr, AmmoniteCompletion* completion) {
        //Add work to the queue
        workQueue->push(work, userPtr, completion);

        //Increase job count, wake a sleeping thread
        jobCount++;
        jobCount.notify_one();
      }

      //Submit multiple jobs without locking multiple times
      void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                          AmmoniteCompletion* completions, int newJobs) {
        workQueue->pushMultiple(work, userBuffer, stride, completions, newJobs);
        jobCount += newJobs;
        jobCount.notify_all();
      }

      //Create thread pool, existing work will begin executing
      int createThreadPool(unsigned int threadCount) {
        //Exit if thread pool already exists
        if (poolThreadCount != 0) {
          return -1;
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
          return -1;
        }

        //Create the threads for the pool
        stayAlive = true;
        for (unsigned int i = 0; i < threadCount; i++) {
          threadPool[i] = std::thread(initWorker);
        }

        unblockThreadsTrigger.test_and_set();
        threadsUnblockedFlag.test_and_set();
        blockedThreadCount = 0;
        poolThreadCount = threadCount;
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
        for (unsigned int i = 0; i < poolThreadCount; i++) {
          workQueue->push(blocker, nullptr, nullptr);
        }

        //Add to job count and wake all threads
        jobCount += poolThreadCount;
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

        if (jobCount != 0) {
          issuesFound = true;
          if (verbose) {
            ammoniteInternalDebug << "WARNING: Job count is non-zero (" \
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

        //Kill all threads after they're unblocked and wake up
        stayAlive = false;

        //Unblock threads and wake them up
        unblockThreads(true);

        //Wait until all threads are done
        for (unsigned int i = 0; i < poolThreadCount; i++) {
          try {
            threadPool[i].join();
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
        delete [] threadPool;
        poolThreadCount = 0;
      }
    }
  }
}

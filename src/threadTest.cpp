#include <atomic>
#include <cstdlib>
#include <iostream>

#include "ammonite/ammonite.hpp"
#include "ammonite/utils/internal/threadPool.hpp"

#define JOB_COUNT (2 << 16)

#define CREATE_THREAD_POOL(THREADS) \
if (!ammonite::utils::thread::internal::createThreadPool((THREADS))) { \
  ammonite::utils::error << "Failed to create thread pool, exiting" << std::endl; \
  return false; \
}

#ifdef DEBUG
  #define DESTROY_THREAD_POOL \
  ammonite::utils::thread::internal::destroyThreadPool(); \
  if (ammonite::utils::thread::internal::debugCheckRemainingWork(false)) { \
    passed = false; \
  }
#else
  #define DESTROY_THREAD_POOL \
  ammonite::utils::thread::internal::destroyThreadPool();
#endif

#define INIT_TIMERS \
ammonite::utils::Timer submitTimer; \
ammonite::utils::Timer runTimer; \
ammonite::utils::Timer totalTimer;

#define FINISH_TIMERS \
runTimer.pause(); \
totalTimer.pause(); \
std::cout << "  Submit done : " << submitTimer.getTime() << "s" << std::endl; \
std::cout << "  Finish work : " << runTimer.getTime() << "s" << std::endl; \
std::cout << "  Total time  : " << totalTimer.getTime() << "s" << std::endl;

#define RESET_TIMERS \
submitTimer.reset(); \
runTimer.reset(); \
totalTimer.reset();

#define SUBMIT_JOBS(jobCount) \
bool passed = true; \
int* values = new int[(jobCount)]{}; \
for (int i = 0; i < (jobCount); i++) { \
  ammonite::utils::thread::submitWork(shortTask, &(values)[i], nullptr); \
}

#define SUBMIT_SYNC_JOBS(jobCount, group) \
bool passed = true; \
int* values = new int[(jobCount)]{}; \
for (int i = 0; i < (jobCount); i++) { \
  ammonite::utils::thread::submitWork(shortTask, &(values)[i], &group); \
}

#define VERIFY_WORK(jobCount) \
for (int i = 0; i < (jobCount); i++) { \
  if (values[i] != 1) { \
    passed = false; \
    ammonite::utils::error << "Failed to verify work (index " << i << ")" << std::endl; \
    break; \
  } \
} \
delete [] values;

namespace {
  struct ResubmitData {
    int* writePtr;
    AmmoniteGroup* syncPtr;
  };

  struct ChainData {
    std::atomic<int> totalSubmitted;
    int targetSubmitted;
    AmmoniteWork work;
    AmmoniteGroup* syncPtr;
  };

  static void shortTask(void* userPtr) {
    *(int*)userPtr = 1;
  }

  static void resubmitTask(void* userPtr) {
    ResubmitData* dataPtr = (ResubmitData*)userPtr;
    ammonite::utils::thread::submitWork(shortTask, dataPtr->writePtr, dataPtr->syncPtr);
  }

  static void chainTask(void* userPtr) {
    ChainData* dataPtr = (ChainData*)userPtr;
    if (dataPtr->totalSubmitted != dataPtr->targetSubmitted) {
      dataPtr->totalSubmitted++;
      ammonite::utils::thread::submitWork(chainTask, dataPtr, dataPtr->syncPtr);
    }
  }
}

namespace {
  static bool testCreateSubmitWaitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS(jobCount, group)
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testCreateSubmitBlockUnblockDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_JOBS(jobCount)
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testCreateSubmitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_JOBS(jobCount)
    submitTimer.pause();

    //Finish work
    DESTROY_THREAD_POOL
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    return passed;
  }

  static bool testCreateBlockSubmitUnblockWaitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    AmmoniteGroup group{0};

    ammonite::utils::thread::blockThreads();

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS(jobCount, group)
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testQueueLimits(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    jobCount *= 4;
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS(jobCount, group)
    submitTimer.pause();
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    VERIFY_WORK(jobCount)

    //Submit second batch
    values = new int[(jobCount)]{};
    submitTimer.unpause();
    for (int i = 0; i < jobCount; i++) { \
      ammonite::utils::thread::submitWork(shortTask, &(values)[i], &group); \
    }
    submitTimer.pause();

    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testNestedJobs(int fullJobCount) {
    int jobCount = fullJobCount / 2;
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    AmmoniteGroup group{0};

    //Submit nested 'jobs'
    RESET_TIMERS
    bool passed = true;
    int* values = new int[jobCount]{};
    ResubmitData* data = new ResubmitData[jobCount]{};
    for (int i = 0; i < jobCount; i++) {
      data[i].writePtr = &values[i];
      data[i].syncPtr = &group;
      ammonite::utils::thread::submitWork(resubmitTask, &data[i], nullptr);
    }
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    delete [] data;
    VERIFY_WORK(jobCount)
    DESTROY_THREAD_POOL

    return passed;
  }

  static bool testChainJobs(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    AmmoniteGroup sync{0};
    ChainData userData = {1, jobCount, chainTask, &sync};
    ammonite::utils::thread::submitWork(chainTask, &userData, &sync);
    submitTimer.pause();

    ammonite::utils::thread::waitGroupComplete(&sync, jobCount);
    FINISH_TIMERS

    bool passed = true;
    if (userData.totalSubmitted != jobCount) {
      passed = false;
      ammonite::utils::error << "Failed to verify work" << std::endl;
    }

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testSubmitMultiple(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    int* values = new int[jobCount]{};
    AmmoniteGroup group{0};
    ammonite::utils::thread::submitMultiple(shortTask, (void*)&values[0], sizeof(int),
      &group, jobCount);
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testSubmitMultipleMultiple(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    int* values = new int[jobCount * 4]{};
    AmmoniteGroup group{0};
    for (int i = 0; i < 4; i++) {
      ammonite::utils::thread::submitMultiple(shortTask, (void*)&values[jobCount * i],
        sizeof(int), &group, jobCount);
    }
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount * 4);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testSubmitMultipleNoSync(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    int* values = new int[jobCount]{};
    ammonite::utils::thread::submitMultiple(shortTask, (void*)&values[0],
                                            sizeof(int), nullptr, jobCount);
    submitTimer.pause();

    //Finish work
    DESTROY_THREAD_POOL
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    return passed;
  }
}

namespace {
  static bool testCreateBlockBlockUnblockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockBlockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockBlockSubmitUnblockDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();

    SUBMIT_JOBS(jobCount)
    ammonite::utils::thread::unblockThreads();

    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockBlockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockUnblockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }
}

int main() {
  bool failed = false;
  ammonite::utils::status << ammonite::utils::thread::internal::getHardwareThreadCount() \
                          << " hardware threads detected" << std::endl;

  //Begin regular tests
  std::cout << "Testing standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(JOB_COUNT);

  std::cout << "Testing alternative sync" << std::endl;
  failed |= !testCreateSubmitBlockUnblockDestroy(JOB_COUNT);

  std::cout << "Testing no sync" << std::endl;
  failed |= !testCreateSubmitDestroy(JOB_COUNT);

  std::cout << "Testing blocked queue" << std::endl;
  failed |= !testCreateBlockSubmitUnblockWaitDestroy(JOB_COUNT);

  std::cout << "Testing queue limits (8x regular over 2 batches)" << std::endl;
  failed |= !testQueueLimits(JOB_COUNT);

  std::cout << "Testing nested jobs" << std::endl;
  failed |= !testNestedJobs(JOB_COUNT);

  std::cout << "Testing chained jobs" << std::endl;
  failed |= !testChainJobs(JOB_COUNT);

  std::cout << "Testing submit multiple" << std::endl;
  failed |= !testSubmitMultiple(JOB_COUNT);

  std::cout << "Testing submit multiple (4x regular over 4 batches)" << std::endl;
  failed |= !testSubmitMultipleMultiple(JOB_COUNT);

  std::cout << "Testing submit multiple, no sync" << std::endl;
  failed |= !testSubmitMultipleNoSync(JOB_COUNT);

  //Begin blocking tests
  std::cout << "Testing double block, double unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockUnblockSubmitDestroy(JOB_COUNT);

  std::cout << "Testing double block, single unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockSubmitDestroy(JOB_COUNT);

  std::cout << "Testing double block, submit jobs, single unblock" << std::endl;
  failed |= !testCreateBlockBlockSubmitUnblockDestroy(JOB_COUNT);

  std::cout << "Testing double block, submit jobs, no explicit unblock" << std::endl;
  failed |= !testCreateBlockBlockSubmitDestroy(JOB_COUNT);

  std::cout << "Testing single block, double unblock" << std::endl;
  failed |= !testCreateBlockUnblockUnblockSubmitDestroy(JOB_COUNT);

  std::cout << "Testing unblock without block" << std::endl;
  failed |= !testCreateUnblockSubmitDestroy(JOB_COUNT);

  //Check system is still functional
  std::cout << "Double-checking standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(JOB_COUNT);

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

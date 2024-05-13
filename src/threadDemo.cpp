#include <atomic>
#include <cstdlib>
#include <iostream>

#include "ammonite/ammonite.hpp"
#include "ammonite/core/threadManager.hpp"

#define JOB_COUNT (2 << 16)

#define CREATE_THREAD_POOL(THREADS) \
if (ammonite::thread::internal::createThreadPool((THREADS)) == -1) { \
  ammonite::utils::error << "Failed to create thread pool, exiting" << std::endl; \
  return false; \
}

#ifdef DEBUG
  #define DESTROY_THREAD_POOL \
  ammonite::thread::internal::destroyThreadPool(); \
  if (ammonite::thread::internal::debugCheckRemainingWork(false)) { \
    passed = false; \
  }
#else
  #define DESTROY_THREAD_POOL \
  ammonite::thread::internal::destroyThreadPool();
#endif

#define PREP_SYNC(jobCount, syncs) \
std::atomic_flag* syncs = new std::atomic_flag[(jobCount)]{ATOMIC_FLAG_INIT};

#define SYNC_THREADS(jobCount, syncs) \
for (int i = 0; i < (jobCount); i++) { \
  ammonite::thread::waitWorkComplete(&(syncs)[i]); \
} \
delete [] (syncs);

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
  ammonite::thread::submitWork(shortTask, &(values)[i], nullptr); \
}

#define SUBMIT_SYNC_JOBS(jobCount, syncs) \
bool passed = true; \
int* values = new int[(jobCount)]{}; \
for (int i = 0; i < (jobCount); i++) { \
  ammonite::thread::submitWork(shortTask, &(values)[i], &(syncs)[i]); \
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

static void shortTask(void* userPtr) {
  *(int*)userPtr = 1;
}

struct ResubmitData {
  int* writePtr;
  std::atomic_flag* syncPtr;
};

static void resubmitTask(void* userPtr) {
  ResubmitData* dataPtr = (ResubmitData*)userPtr;
  ammonite::thread::submitWork(shortTask, dataPtr->writePtr, dataPtr->syncPtr);
}

namespace {
  static bool testCreateSubmitWaitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    PREP_SYNC(jobCount, syncs)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS(jobCount, syncs)
    submitTimer.pause();

    //Finish work
    SYNC_THREADS(jobCount, syncs)
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
    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();
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
    PREP_SYNC(jobCount, syncs)

    ammonite::thread::blockThreadsSync();

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS(jobCount, syncs)
    submitTimer.pause();

    //Finish work
    ammonite::thread::unblockThreadsSync();
    SYNC_THREADS(jobCount, syncs)
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testQueueLimits(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    jobCount *= 4;
    PREP_SYNC(jobCount, syncs)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS(jobCount, syncs)
    submitTimer.pause();
    SYNC_THREADS(jobCount, syncs)
    VERIFY_WORK(jobCount)

    //Submit second batch
    syncs = new std::atomic_flag[(jobCount)]{ATOMIC_FLAG_INIT};
    values = new int[(jobCount)]{};
    submitTimer.unpause();
    for (int i = 0; i < jobCount; i++) { \
      ammonite::thread::submitWork(shortTask, &(values)[i], &(syncs)[i]); \
    }
    submitTimer.pause();

    SYNC_THREADS(jobCount, syncs)
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    DESTROY_THREAD_POOL
    return passed;
  }

  static bool testNestedJobs(int fullJobCount) {
    int jobCount = fullJobCount / 2;
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    PREP_SYNC(jobCount, syncs)

    //Submit nested 'jobs'
    RESET_TIMERS
    bool passed = true;
    int* values = new int[jobCount]{};
    ResubmitData* data = new ResubmitData[jobCount]{};
    for (int i = 0; i < jobCount; i++) {
      data[i].writePtr = &values[i];
      data[i].syncPtr = &syncs[i];
      ammonite::thread::submitWork(resubmitTask, &data[i], nullptr);
    }
    submitTimer.pause();

    //Finish work
    SYNC_THREADS(jobCount, syncs)
    FINISH_TIMERS
    delete [] data;
    VERIFY_WORK(jobCount)
    DESTROY_THREAD_POOL

    return passed;
  }

  static bool testSubmitMultiple(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    PREP_SYNC(jobCount, syncs)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    int* values = new int[jobCount]{};
    ammonite::thread::submitMultiple(shortTask, (void*)&values[0],
                                     sizeof(int), &syncs[0], jobCount);
    submitTimer.pause();

    //Finish work
    SYNC_THREADS(jobCount, syncs)
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
    ammonite::thread::submitMultiple(shortTask, (void*)&values[0],
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

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();
    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockBlockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockBlockSubmitUnblockDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();

    SUBMIT_JOBS(jobCount)
    ammonite::thread::unblockThreadsSync();

    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockBlockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateBlockUnblockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();
    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }

  static bool testCreateUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS(jobCount)
    DESTROY_THREAD_POOL
    VERIFY_WORK(jobCount)
    return passed;
  }
}

int main() {
  bool failed = false;
  ammonite::utils::status << ammonite::thread::internal::getHardwareThreadCount() \
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

  std::cout << "Testing queue limits (8x regular in 2 batches)" << std::endl;
  failed |= !testQueueLimits(JOB_COUNT);

  std::cout << "Testing nested jobs" << std::endl;
  failed |= !testNestedJobs(JOB_COUNT);

  std::cout << "Testing submit multiple" << std::endl;
  failed |= !testSubmitMultiple(JOB_COUNT);

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

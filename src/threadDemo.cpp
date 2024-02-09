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
      return false; \
    }
#else
  #define DESTROY_THREAD_POOL \
  ammonite::thread::internal::destroyThreadPool();
#endif

#define PREP_SYNC(jobCount, syncs) \
std::atomic_flag* syncs = (std::atomic_flag*)std::malloc(sizeof(std::atomic_flag) * (jobCount));

#define SYNC_THREADS(jobCount, syncs) \
for (int i = 0; i < (jobCount); i++) { \
  ammonite::thread::waitWorkComplete(&(syncs)[i]); \
} \
std::free((syncs));

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

#define SUBMIT_JOBS \
for (int i = 0; i < jobCount; i++) { \
  ammonite::thread::submitWork(shortTask, nullptr, nullptr); \
}

#define SUBMIT_SYNC_JOBS \
for (int i = 0; i < jobCount; i++) { \
  ammonite::thread::submitWork(shortTask, nullptr, &syncs[i]); \
}

static void shortTask(void*) {
  return;
}

namespace {
  static bool testCreateSubmitWaitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    PREP_SYNC(jobCount, syncs)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS
    submitTimer.pause();

    //Finish work
    SYNC_THREADS(jobCount, syncs)
    FINISH_TIMERS

    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateSubmitBlockUnblockDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_JOBS
    submitTimer.pause();

    //Finish work
    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();
    FINISH_TIMERS

    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateSubmitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_JOBS
    submitTimer.pause();

    //Finish work
    DESTROY_THREAD_POOL
    FINISH_TIMERS

    return true;
  }

  static bool testCreateBlockSubmitUnblockWaitDestroy(int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    PREP_SYNC(jobCount, syncs)

    ammonite::thread::blockThreadsSync();

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_SYNC_JOBS
    submitTimer.pause();

    //Finish work
    ammonite::thread::unblockThreadsSync();
    SYNC_THREADS(jobCount, syncs)
    FINISH_TIMERS

    DESTROY_THREAD_POOL
    return true;
  }

  static bool testSubmitCreateWaitDestroy(int jobCount) {
    INIT_TIMERS
    PREP_SYNC(jobCount, syncs)

    //Submit fast 'jobs'
    submitTimer.reset();
    totalTimer.reset();
    SUBMIT_SYNC_JOBS
    submitTimer.pause();

    runTimer.reset();
    CREATE_THREAD_POOL(0)

    //Finish work
    SYNC_THREADS(jobCount, syncs)
    FINISH_TIMERS

    DESTROY_THREAD_POOL
    return true;
  }
}

namespace {
  static bool testCreateBlockBlockUnblockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();
    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS
    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateBlockBlockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS
    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateBlockBlockSubmitUnblockDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();

    SUBMIT_JOBS
    ammonite::thread::unblockThreadsSync();

    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateBlockBlockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::blockThreadsSync();

    SUBMIT_JOBS
    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateBlockUnblockUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::blockThreadsSync();
    ammonite::thread::unblockThreadsSync();
    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS
    DESTROY_THREAD_POOL
    return true;
  }

  static bool testCreateUnblockSubmitDestroy(int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::thread::unblockThreadsSync();

    SUBMIT_JOBS
    DESTROY_THREAD_POOL
    return true;
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

  std::cout << "Testing pre-filled queue" << std::endl;
  failed |= !testSubmitCreateWaitDestroy(JOB_COUNT);

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

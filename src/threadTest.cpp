#include <atomic>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include <ammonite/ammonite.hpp>

//NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define CREATE_THREAD_POOL(THREADS) \
if (!ammonite::utils::thread::createThreadPool((THREADS))) { \
  ammonite::utils::error << "Failed to create thread pool, exiting" << std::endl; \
  return false; \
}

#define INIT_TIMERS \
ammonite::utils::Timer submitTimer; \
ammonite::utils::Timer runTimer; \
ammonite::utils::Timer totalTimer;

#define FINISH_TIMERS \
runTimer.pause(); \
totalTimer.pause(); \
output << "  Submit done : " << submitTimer.getTime() << "s" << std::endl; \
output << "  Finish work : " << runTimer.getTime() << "s" << std::endl; \
output << "  Total time  : " << totalTimer.getTime() << "s" << std::endl;

#define RESET_TIMERS \
submitTimer.reset(); \
runTimer.reset(); \
totalTimer.reset();

#define SUBMIT_JOBS(jobCount) \
bool passed = true; \
unsigned int* values = new unsigned int[(jobCount)]{}; \
for (unsigned int i = 0; i < (jobCount); i++) { \
  ammonite::utils::thread::submitWork(shortTask, &(values)[i], nullptr); \
}

#define SUBMIT_SYNC_JOBS(jobCount, group) \
bool passed = true; \
unsigned int* values = new unsigned int[(jobCount)]{}; \
for (unsigned int i = 0; i < (jobCount); i++) { \
  ammonite::utils::thread::submitWork(shortTask, &(values)[i], &(group)); \
}

#define VERIFY_WORK(jobCount) \
for (unsigned int i = 0; i < (jobCount); i++) { \
  if (values[i] != 1) { \
    passed = false; \
    ammonite::utils::error << "Failed to verify work (index " << i << ")" << std::endl; \
    break; \
  } \
} \
delete [] values;
//NOLINTEND(cppcoreguidelines-macro-usage)

namespace {
  //NOLINTBEGIN(cppcoreguidelines-interfaces-global-init)
  std::stringstream outputCapture("");
  ammonite::utils::OutputHelper outputTester(outputCapture, "PREFIX: ");

  //Thread-safe output
  ammonite::utils::OutputHelper output(std::cout, "", ammonite::utils::colour::none);
  //NOLINTEND(cppcoreguidelines-interfaces-global-init)
}

namespace {
  struct ResubmitData {
    unsigned int* writePtr;
    AmmoniteGroup* syncPtr;
  };

  struct ChainData {
    std::atomic<unsigned int> totalSubmitted;
    unsigned int targetSubmitted;
    AmmoniteWork work;
    AmmoniteGroup* syncPtr;
  };

  void shortTask(void* userPtr) {
    *(unsigned int*)userPtr = 1;
  }

  void resubmitTask(void* userPtr) {
    const ResubmitData* dataPtr = (ResubmitData*)userPtr;
    ammonite::utils::thread::submitWork(shortTask, dataPtr->writePtr, dataPtr->syncPtr);
  }

  void chainTask(void* userPtr) {
    ChainData* dataPtr = (ChainData*)userPtr;
    if (dataPtr->totalSubmitted != dataPtr->targetSubmitted) {
      dataPtr->totalSubmitted++;
      ammonite::utils::thread::submitWork(chainTask, dataPtr, dataPtr->syncPtr);
    }
  }

  constexpr unsigned int outputCount = 1000;
  void loggingTask(void* userPtr) {
    const unsigned int value = *((unsigned int*)userPtr);
    outputTester << value << " ";
    for (unsigned int i = 0; i < outputCount; i++) {
      outputTester << value;
    }
    outputTester << std::endl;

    *(unsigned int*)userPtr = 1;
  }
}

namespace {
  bool testCreateSubmitWaitDestroy(unsigned int jobCount) {
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

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testCreateSubmitBlockUnblockDestroy(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_JOBS(jobCount)
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::finishWork();
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testCreateSubmitDestroy(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    SUBMIT_JOBS(jobCount)
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::destroyThreadPool();
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    return passed;
  }

  bool testCreateBlockSubmitUnblockWaitDestroy(unsigned int jobCount) {
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

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testQueueLimits(unsigned int jobCount) {
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
    values = new unsigned int[(jobCount)]{};
    submitTimer.unpause();
    for (unsigned int i = 0; i < jobCount; i++) { \
      ammonite::utils::thread::submitWork(shortTask, &(values)[i], &group); \
    }
    submitTimer.pause();

    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testNestedJobs(int fullJobCount) {
    const unsigned int jobCount = fullJobCount / 2;
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    AmmoniteGroup group{0};

    //Submit nested 'jobs'
    RESET_TIMERS
    bool passed = true;
    unsigned int* values = new unsigned int[jobCount]{};
    ResubmitData* data = new ResubmitData[jobCount]{};
    for (unsigned int i = 0; i < jobCount; i++) {
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
    ammonite::utils::thread::destroyThreadPool();

    return passed;
  }

  bool testChainJobs(unsigned int jobCount) {
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

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testSubmitMultiple(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    unsigned int* values = new unsigned int[jobCount]{};
    AmmoniteGroup group{0};
    ammonite::utils::thread::submitMultiple(shortTask, &values[0], sizeof(values[0]),
                                            &group, jobCount, nullptr);
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testSubmitMultipleMultiple(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    unsigned int* values = new unsigned int[(std::size_t)jobCount * 4]{};
    AmmoniteGroup group{0};
    for (int i = 0; i < 4; i++) {
      ammonite::utils::thread::submitMultiple(shortTask, &values[(std::size_t)jobCount * i],
        sizeof(values[0]), &group, jobCount, nullptr);
    }
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount * 4);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testSubmitMultipleSyncSubmit(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    unsigned int* values = new unsigned int[jobCount]{};
    AmmoniteGroup group{0};
    ammonite::utils::thread::submitMultipleSync(shortTask, &values[0], sizeof(values[0]),
                                                &group, jobCount);
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }

  bool testSubmitMultipleNoSync(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)

    //Submit fast 'jobs'
    RESET_TIMERS
    bool passed = true;
    unsigned int* values = new unsigned int[jobCount]{};
    AmmoniteGroup submitGroup{0};
    ammonite::utils::thread::submitMultiple(shortTask, &values[0],
      sizeof(values[0]), nullptr, jobCount, &submitGroup);
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&submitGroup, 1);
    ammonite::utils::thread::destroyThreadPool();
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    return passed;
  }
}

namespace {
  bool testCreateBlockBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    ammonite::utils::thread::destroyThreadPool();
    VERIFY_WORK(jobCount)
    return passed;
  }

  bool testCreateBlockBlockUnblockSubmitDestroy(unsigned int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    ammonite::utils::thread::destroyThreadPool();
    VERIFY_WORK(jobCount)
    return passed;
  }

  bool testCreateBlockBlockSubmitUnblockDestroy(unsigned int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();

    SUBMIT_JOBS(jobCount)
    ammonite::utils::thread::unblockThreads();

    ammonite::utils::thread::destroyThreadPool();
    VERIFY_WORK(jobCount)
    return passed;
  }

  bool testCreateBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    ammonite::utils::thread::destroyThreadPool();
    VERIFY_WORK(jobCount)
    return passed;
  }

  bool testCreateUnblockSubmitDestroy(unsigned int jobCount) {
    CREATE_THREAD_POOL(0)

    ammonite::utils::thread::unblockThreads();

    SUBMIT_JOBS(jobCount)
    ammonite::utils::thread::destroyThreadPool();
    VERIFY_WORK(jobCount)
    return passed;
  }
}

namespace {
  bool testOutputHelpers(unsigned int jobCount) {
    INIT_TIMERS
    CREATE_THREAD_POOL(0)
    AmmoniteGroup group{0};

    //Submit logging jobs
    RESET_TIMERS
    bool passed = true;
    unsigned int* values = new unsigned int[jobCount];
    for (unsigned int i = 0; i < jobCount; i++) {
      values[i] = i;
      ammonite::utils::thread::submitWork(loggingTask, &values[i], &group);
    }
    submitTimer.pause();

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    FINISH_TIMERS
    VERIFY_WORK(jobCount)

    //Verify output blocks
    std::string threadOutput;
    std::unordered_set<unsigned int> foundValues;
    while (std::getline(outputCapture, threadOutput)) {
      std::stringstream lineStream(threadOutput);
      std::string component;

      //Extract and verify prefix
      std::getline(lineStream, component, ' ');
      if (component != std::string("PREFIX:")) {
        ammonite::utils::error << "Failed to verify output prefix" << std::endl;
        ammonite::utils::error << "Expected: PREFIX:" << std::endl;
        ammonite::utils::error << "Got:" << component << std::endl;
        passed = false;
      }

      //Extract the value used for the data
      std::string value;
      std::getline(lineStream, value, ' ');
      foundValues.insert(stoi(value));

      //Generate expected output block
      std::getline(lineStream, component);
      std::string expected;
      for (unsigned int i = 0; i < outputCount; i++) {
        expected.append(value);
      }

      //Verify output block
      if (component != expected) {
        ammonite::utils::error << "Failed to verify output block" << std::endl;
        ammonite::utils::error << "Expected:" << expected << std::endl;
        ammonite::utils::error << "Got:" << component << std::endl;
        passed = false;
      }
    }

    //Verify all numbers were seen
    for (unsigned int i = 0; i < jobCount; i++) {
      if (!foundValues.contains(i)) {
        ammonite::utils::error << "Failed to verify value '" << i << "'" << std::endl;
        passed = false;
      }
    }

    ammonite::utils::thread::destroyThreadPool();
    return passed;
  }
}

int main() noexcept(false) {
  bool failed = false;
  ammonite::utils::status << ammonite::utils::thread::getHardwareThreadCount() \
                          << " hardware threads detected" << std::endl;

  //Pick jobs per test
  const unsigned int jobCount = (2 << 16);

  //Begin regular tests
  output << "Testing standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(jobCount);

  output << "Testing alternative sync" << std::endl;
  failed |= !testCreateSubmitBlockUnblockDestroy(jobCount);

  output << "Testing no sync" << std::endl;
  failed |= !testCreateSubmitDestroy(jobCount);

  output << "Testing blocked queue" << std::endl;
  failed |= !testCreateBlockSubmitUnblockWaitDestroy(jobCount);

  output << "Testing queue limits (8x regular over 2 batches)" << std::endl;
  failed |= !testQueueLimits(jobCount);

  output << "Testing nested jobs" << std::endl;
  failed |= !testNestedJobs(jobCount);

  output << "Testing chained jobs" << std::endl;
  failed |= !testChainJobs(jobCount);

  output << "Testing submit multiple" << std::endl;
  failed |= !testSubmitMultiple(jobCount);

  output << "Testing submit multiple, minimal" << std::endl;
  failed |= !testSubmitMultiple(1);

  output << "Testing submit multiple, thread count" << std::endl;
  failed |= !testSubmitMultiple(ammonite::utils::thread::getThreadPoolSize());

  output << "Testing submit multiple (4x regular over 4 batches)" << std::endl;
  failed |= !testSubmitMultipleMultiple(jobCount);

  output << "Testing submit multiple, synchronous submit" << std::endl;
  failed |= !testSubmitMultipleSyncSubmit(jobCount);

  output << "Testing submit multiple, no job sync" << std::endl;
  failed |= !testSubmitMultipleNoSync(jobCount);

  //Begin blocking tests
  output << "Testing double block, double unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockUnblockSubmitDestroy(jobCount);

  output << "Testing double block, single unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockSubmitDestroy(jobCount);

  output << "Testing double block, submit jobs, single unblock" << std::endl;
  failed |= !testCreateBlockBlockSubmitUnblockDestroy(jobCount);

  output << "Testing single block, double unblock" << std::endl;
  failed |= !testCreateBlockUnblockUnblockSubmitDestroy(jobCount);

  output << "Testing unblock without block" << std::endl;
  failed |= !testCreateUnblockSubmitDestroy(jobCount);

  //Check system is still functional
  output << "Double-checking standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(jobCount);

  output << "Testing synchronised output helpers" << std::endl;
  const unsigned int threadCount = ammonite::utils::thread::getHardwareThreadCount();
  failed |= !testOutputHelpers(threadCount * 4);

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

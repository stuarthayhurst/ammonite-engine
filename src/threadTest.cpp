#include <atomic>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <ammonite/ammonite.hpp>

namespace {
  //NOLINTBEGIN(cppcoreguidelines-interfaces-global-init)
  std::stringstream outputCapture("");
  ammonite::utils::OutputHelper outputTester(outputCapture, "PREFIX: ");
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
    unsigned int* values;
    AmmoniteGroup* syncPtr;
  };

  void shortTask(void* userPtr) {
    *(unsigned int*)userPtr = 1;
  }

  void resubmitTask(void* userPtr) {
    const ResubmitData* const dataPtr = (ResubmitData*)userPtr;
    ammonite::utils::thread::submitWork(shortTask, dataPtr->writePtr, dataPtr->syncPtr);
  }

  void chainTask(void* userPtr) {
    ChainData* const dataPtr = (ChainData*)userPtr;
    *(dataPtr->values++) = 1;
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

  void blockingTask(void* userPtr) {
    const std::atomic_flag* const flagPtr = (std::atomic_flag*)userPtr;
    flagPtr->wait(false);
  }
}

namespace {
  bool createThreadPool(unsigned int threadCount) {
    if (!ammonite::utils::thread::createThreadPool(threadCount)) {
      ammonite::utils::error << "Failed to create thread pool, exiting" << std::endl;
      return false;
    }

    return true;
  }

  void destroyThreadPool() {
    ammonite::utils::thread::destroyThreadPool();
  }

  ammonite::utils::Timer* createTimers() {
    return new ammonite::utils::Timer[3];
  }

  void destroyTimers(ammonite::utils::Timer* timers) {
    delete [] timers;
  }

  void resetTimers(ammonite::utils::Timer* timers) {
    for (int i = 0; i < 3; i++) {
      timers[i].reset();
    }
  }

  void resumeSubmitTimer(ammonite::utils::Timer* timers) {
    timers[0].unpause();
  }

  void finishSubmitTimer(ammonite::utils::Timer* timers) {
    timers[0].pause();
  }

  void finishExecutionTimers(ammonite::utils::Timer* timers) {
    timers[1].pause();
    timers[2].pause();
  }

  void printTimers(ammonite::utils::Timer* timers) {
    ammonite::utils::normal << "  Submit done : " << timers[0].getTime() << "s" << std::endl;
    ammonite::utils::normal << "  Finish work : " << timers[1].getTime() << "s" << std::endl;
    ammonite::utils::normal << "  Total time  : " << timers[2].getTime() << "s" << std::endl;
  }

  unsigned int* createValues(unsigned int jobCount) {
    return new unsigned int[jobCount]{};
  }

  void destroyValues(const unsigned int* values) {
    delete [] values;
  }

  void submitShortJobs(unsigned int jobCount, unsigned int* values) {
    for (unsigned int i = 0; i < jobCount; i++) { \
      ammonite::utils::thread::submitWork(shortTask, &values[i], nullptr); \
    }
  }

  void submitShortSyncJobs(unsigned int jobCount, unsigned int* values, AmmoniteGroup* group) {
    for (unsigned int i = 0; i < jobCount; i++) { \
      ammonite::utils::thread::submitWork(shortTask, &values[i], group); \
    }
  }

  bool verifyWork(unsigned int jobCount, const unsigned int* values) {
    for (unsigned int i = 0; i < jobCount; i++) {
      if (values[i] != 1) {
        ammonite::utils::error << "Failed to verify work (index " << i << ")" << std::endl;
        return false;
      }
    }

    return true;
  }
}

namespace {
  bool testCreateSubmitWaitDestroy(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testCreateSubmitBlockUnblockDestroy(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortJobs(jobCount, values);
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::finishWork();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testCreateSubmitDestroy(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortJobs(jobCount, values);
    finishSubmitTimer(timers);

    //Finish work
    destroyThreadPool();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyTimers(timers);
    return passed;
  }

  bool testCreateBlockSubmitUnblockWaitDestroy(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup group{0};

    ammonite::utils::thread::blockThreads();

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testQueueLimits(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    jobCount *= 4;
    unsigned int* values = createValues(jobCount);
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Clean up after the first batch
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    bool passed = verifyWork(jobCount, values);
    destroyValues(values);

    //Submit second batch
    values = createValues(jobCount);
    resumeSubmitTimer(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Clean up after the second batch
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    passed &= verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testNestedJobs(int fullJobCount) {
    const unsigned int jobCount = fullJobCount / 2;
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup group{0};

    //Submit nested 'jobs'
    resetTimers(timers);
    ResubmitData* const data = new ResubmitData[jobCount]{};
    for (unsigned int i = 0; i < jobCount; i++) {
      data[i].writePtr = &values[i];
      data[i].syncPtr = &group;
      ammonite::utils::thread::submitWork(resubmitTask, &data[i], nullptr);
    }
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    delete [] data;
    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testChainJobs(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    const unsigned int poolSize = ammonite::utils::thread::getThreadPoolSize();
    const unsigned int totalJobCount = jobCount * poolSize;
    unsigned int* const values = createValues(totalJobCount);
    AmmoniteGroup sync{0};

    ChainData* const userDataArray = new ChainData[poolSize];
    for (std::size_t i = 0; i < poolSize; i++) {
      userDataArray[i].totalSubmitted = 1;
      userDataArray[i].targetSubmitted = jobCount;
      userDataArray[i].work = chainTask;
      userDataArray[i].values = values + (i * jobCount);
      userDataArray[i].syncPtr = &sync;
    }

    //Submit chain 'jobs'
    resetTimers(timers);
    ammonite::utils::thread::submitMultiple(chainTask, userDataArray, sizeof(ChainData),
                                            &sync, poolSize, nullptr);
    finishSubmitTimer(timers);

    ammonite::utils::thread::waitGroupComplete(&sync, totalJobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(totalJobCount, values);

    delete [] userDataArray;
    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSingleSyncHelper(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    AmmoniteGroup group{0};

    //Test single work complete check before work is submitted
    if (ammonite::utils::thread::isSingleWorkComplete(&group)) {
      ammonite::utils::error << "Single work incorrectly reported as complete" << std::endl;
      return false;
    }

    //Prepare flags to control job flow
    std::atomic_flag* const flags = new std::atomic_flag[jobCount]{ATOMIC_FLAG_INIT};

    //Submit controlled blocking jobs
    resetTimers(timers);
    for (unsigned int i = 0; i < jobCount; i++) {
      ammonite::utils::thread::submitWork(blockingTask, &flags[i], &group);
    }
    finishSubmitTimer(timers);

    //Process the work
    for (unsigned int i = 0; i < jobCount; i++) {
      //Check not jobs have unexpectedly finished
      if (ammonite::utils::thread::isSingleWorkComplete(&group)) {
        ammonite::utils::error << "Single work incorrectly reported as complete" << std::endl;
        delete [] flags;
        return false;
      }

      //Allow a job to progress
      flags[i].test_and_set();
      flags[i].notify_all();

      //Spin until the job finishes
      while (!ammonite::utils::thread::isSingleWorkComplete(&group)) {};
    }

    finishExecutionTimers(timers);
    printTimers(timers);

    delete [] flags;
    destroyThreadPool();
    destroyTimers(timers);
    return true;
  }

  bool testMultipleSyncHelper(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup group{0};

    //Test multiple work complete check before work is submitted
    if (ammonite::utils::thread::getRemainingWork(&group, jobCount) != jobCount) {
      ammonite::utils::error << "Work incorrectly reported as complete" << std::endl;
      return false;
    }

    //Submit 'fast' jobs
    resetTimers(timers);
    ammonite::utils::thread::submitMultiple(shortTask, &values[0], sizeof(values[0]),
                                            &group, jobCount, nullptr);
    finishSubmitTimer(timers);

    //Wait for the jobs to complete
    unsigned int remainingJobs = jobCount;
    while (remainingJobs != 0) {
      remainingJobs = ammonite::utils::thread::getRemainingWork(&group, remainingJobs);
    }

    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultiple(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    ammonite::utils::thread::submitMultiple(shortTask, &values[0], sizeof(values[0]),
                                            &group, jobCount, nullptr);
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleMultiple(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount * 4);
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    for (int i = 0; i < 4; i++) {
      ammonite::utils::thread::submitMultiple(shortTask, &values[(std::size_t)jobCount * i],
        sizeof(values[0]), &group, jobCount, nullptr);
    }
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount * 4);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount * 4, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleSyncSubmit(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    ammonite::utils::thread::submitMultipleSync(shortTask, &values[0], sizeof(values[0]),
                                                &group, jobCount);
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleNoSync(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    AmmoniteGroup submitGroup{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    ammonite::utils::thread::submitMultiple(shortTask, &values[0],
      sizeof(values[0]), nullptr, jobCount, &submitGroup);
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&submitGroup, 1);
    destroyThreadPool();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyTimers(timers);
    return passed;
  }

  bool testRandomWorkloads(unsigned int batchSize) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    const unsigned int testCount = 15;
    const unsigned int totalJobCount = batchSize * testCount;
    unsigned int* const values = createValues(totalJobCount);

    struct BatchInfo {
      AmmoniteGroup* group;
      unsigned int waitCount = 0;
    };

    resetTimers(timers);
    std::vector<BatchInfo> batchInfoVector;
    std::vector<ChainData*> chainDataVector;
    for (std::size_t testIndex = 0; testIndex < testCount; testIndex++) {
      const unsigned int jobTypeCount = 7;
      unsigned int* const offsetValues = values + (testIndex * batchSize);

      switch (ammonite::utils::random<unsigned int>(jobTypeCount - 1)) {
      case 0:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing regular submit, with explicit sync" \
                                << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new AmmoniteGroup{0};

          submitShortSyncJobs(batchSize, offsetValues, batchInfo.group);
          break;
        }
      case 1:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing regular submit, without explicit sync" \
                                << std::endl;
        submitShortSyncJobs(batchSize, offsetValues, nullptr);
        break;
      case 2:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing submit multiple, sync on jobs" \
                                << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new AmmoniteGroup{0};

          ammonite::utils::thread::submitMultiple(shortTask, offsetValues,
                                                  sizeof(values[0]), batchInfo.group,
                                                  batchSize, nullptr);
          break;
        }
      case 3:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing submit multiple, sync on submit" \
                                << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = 1;
          batchInfo.group = new AmmoniteGroup{0};

          ammonite::utils::thread::submitMultiple(shortTask, offsetValues,
                                                  sizeof(values[0]), nullptr,
                                                  batchSize, batchInfo.group);
          break;
        }
      case 4:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing submit multiple, synchronous submit" \
                                << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new AmmoniteGroup{0};

          ammonite::utils::thread::submitMultipleSync(shortTask, offsetValues,
                                                      sizeof(values[0]), batchInfo.group,
                                                      batchSize);
          break;
        }
      case 5:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing submit multiple, blocked" << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new AmmoniteGroup{0};

          ammonite::utils::thread::blockThreads();
          ammonite::utils::thread::submitMultiple(shortTask, offsetValues,
                                                  sizeof(values[0]), batchInfo.group,
                                                  batchSize, nullptr);
          ammonite::utils::thread::unblockThreads();
          break;
        }
      case 6:
        ammonite::utils::normal << "  " << testIndex \
                                << ": Testing chained jobs" << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new AmmoniteGroup{0};

          ChainData* const chainData = new ChainData{
            .totalSubmitted = 1, .targetSubmitted = batchSize, .work = chainTask,
            .values = offsetValues, .syncPtr = batchInfo.group};
          chainDataVector.push_back(chainData);
          ammonite::utils::thread::submitWork(chainTask, chainData, batchInfo.group);
          break;
        }
      default:
        std::unreachable();
        break;
      }
    }
    finishSubmitTimer(timers);

    //Wait for each batch to finish
    for (const BatchInfo& batchInfo : batchInfoVector) {
      ammonite::utils::thread::waitGroupComplete(batchInfo.group,
                                                 batchInfo.waitCount);
      delete batchInfo.group;
    }

    //Clean up chain data
    for (ChainData* const& chainData : chainDataVector) {
      delete chainData;
    }

    ammonite::utils::thread::finishWork();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(totalJobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }
}

namespace {
  bool testOutputHelpers(unsigned int jobCount) {
    ammonite::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    AmmoniteGroup group{0};

    //Submit logging jobs
    resetTimers(timers);
    unsigned int* const values = createValues(jobCount);
    for (unsigned int i = 0; i < jobCount; i++) {
      values[i] = i;
      ammonite::utils::thread::submitWork(loggingTask, &values[i], &group);
    }
    finishSubmitTimer(timers);

    //Finish work
    ammonite::utils::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    bool passed = verifyWork(jobCount, values);

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

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }
}

namespace {
  bool testCreateBlockBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateBlockBlockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateBlockBlockSubmitUnblockDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::blockThreads();

    submitShortJobs(jobCount, values);
    ammonite::utils::thread::unblockThreads();

    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    ammonite::utils::thread::blockThreads();
    ammonite::utils::thread::unblockThreads();
    ammonite::utils::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    ammonite::utils::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
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
  ammonite::utils::normal << "Testing standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(jobCount);

  ammonite::utils::normal << "Testing alternative sync" << std::endl;
  failed |= !testCreateSubmitBlockUnblockDestroy(jobCount);

  ammonite::utils::normal << "Testing no sync" << std::endl;
  failed |= !testCreateSubmitDestroy(jobCount);

  ammonite::utils::normal << "Testing blocked queue" << std::endl;
  failed |= !testCreateBlockSubmitUnblockWaitDestroy(jobCount);

  ammonite::utils::normal << "Testing queue limits (8x regular over 2 batches)" << std::endl;
  failed |= !testQueueLimits(jobCount);

  ammonite::utils::normal << "Testing nested jobs" << std::endl;
  failed |= !testNestedJobs(jobCount);

  ammonite::utils::normal << "Testing chained jobs" << std::endl;
  failed |= !testChainJobs(jobCount);

  ammonite::utils::normal << "Testing single synchronisation helper" << std::endl;
  failed |= !testSingleSyncHelper(jobCount);

  ammonite::utils::normal << "Testing multiple synchronisation helpers" << std::endl;
  failed |= !testMultipleSyncHelper(jobCount);

  ammonite::utils::normal << "Testing submit multiple" << std::endl;
  failed |= !testSubmitMultiple(jobCount);

  ammonite::utils::normal << "Testing submit multiple, minimal" << std::endl;
  failed |= !testSubmitMultiple(1);

  ammonite::utils::normal << "Testing submit multiple, thread count" << std::endl;
  failed |= !testSubmitMultiple(ammonite::utils::thread::getThreadPoolSize());

  ammonite::utils::normal << "Testing submit multiple (4x regular over 4 batches)" << std::endl;
  failed |= !testSubmitMultipleMultiple(jobCount);

  ammonite::utils::normal << "Testing submit multiple, synchronous submit" << std::endl;
  failed |= !testSubmitMultipleSyncSubmit(jobCount);

  ammonite::utils::normal << "Testing submit multiple, no job sync" << std::endl;
  failed |= !testSubmitMultipleNoSync(jobCount);

  ammonite::utils::normal << "Testing random workloads" << std::endl;
  failed |= !testRandomWorkloads(jobCount);

  ammonite::utils::normal << "Testing synchronised output helpers" << std::endl;
  const unsigned int threadCount = ammonite::utils::thread::getHardwareThreadCount();
  failed |= !testOutputHelpers(threadCount * 4);

  //Begin blocking tests
  ammonite::utils::normal << "Testing double block, double unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockUnblockSubmitDestroy(jobCount);

  ammonite::utils::normal << "Testing double block, single unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockSubmitDestroy(jobCount);

  ammonite::utils::normal << "Testing double block, submit jobs, single unblock" << std::endl;
  failed |= !testCreateBlockBlockSubmitUnblockDestroy(jobCount);

  ammonite::utils::normal << "Testing single block, double unblock" << std::endl;
  failed |= !testCreateBlockUnblockUnblockSubmitDestroy(jobCount);

  ammonite::utils::normal << "Testing unblock without block" << std::endl;
  failed |= !testCreateUnblockSubmitDestroy(jobCount);

  //Check system is still functional
  ammonite::utils::normal << "Double-checking standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(jobCount);

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

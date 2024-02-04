#include <cstdlib>

#include "ammonite/ammonite.hpp"
#include "ammonite/core/threadManager.hpp"

#include <atomic>

#include <thread>
#include <chrono>
#include <iostream>



void shortTask(void*) {
  return;
}

int main() {
  ammonite::utils::Timer runTimer;
  if (ammonite::thread::internal::createThreadPool(0) == -1) {
    ammonite::utils::error << "Failed to create thread pool, exiting" << std::endl;
    return EXIT_FAILURE;
  }

  ammonite::utils::status << "Created thread pool with " \
                          << ammonite::thread::getThreadPoolSize() \
                          << " thread(s)" << std::endl;

  //Submit 300000 fast 'jobs'
  long long int numJobs = 300000;
  std::atomic_flag* syncs = (std::atomic_flag*)std::malloc(sizeof(std::atomic_flag) * numJobs);
  for (int i = 0; i < numJobs; i++) {
    ammonite::thread::submitWork(shortTask, nullptr, &syncs[i]);
  }

  //Wait for work to complete
  for (int i = 0; i < numJobs; i++) {
    ammonite::thread::waitWorkComplete(&syncs[i]);
  }
  std::free(syncs);

  std::cout << "Completed in " << runTimer.getTime() << "s" << std::endl;

  //Clean up and exit
  ammonite::thread::internal::destroyThreadPool();
  return EXIT_SUCCESS;
}

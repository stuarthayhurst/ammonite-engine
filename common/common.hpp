#include <chrono>
#include <thread>

void millisleep(int sleepTime) {
  std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
}

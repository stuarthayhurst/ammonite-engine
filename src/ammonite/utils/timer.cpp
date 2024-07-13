#include <chrono>

#include "timer.hpp"

namespace ammonite {
  namespace utils {
    double Timer::getNanoTime() {
      auto systemTime = std::chrono::system_clock::now().time_since_epoch();
      auto systemTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>(systemTime).count();
      return systemTimeNano;
    }

    double Timer::getTimeDelta(double timePoint) {
      return getNanoTime() - timePoint;
    }

    Timer::Timer() {
      isTimerRunning = true;
      startTime = getNanoTime();
      stopTime = 0.0;
      offset = 0.0;
    }

    double Timer::getTime() {
      double runTime;
      if (isTimerRunning) {
        runTime = getNanoTime() - startTime - offset;
      } else {
        //If the timer hasn't been unpaused yet, correct for the time
        runTime = stopTime - startTime - offset;
      }

      return runTime / 1000000000;
    }

    void Timer::setTime(double newTime) {
      double currentTime = getNanoTime();
      startTime = currentTime - (newTime * 1000000000);
      stopTime = currentTime;
      offset = 0.0;
    }

    bool Timer::isRunning() {
      return isTimerRunning;
    }

    void Timer::reset() {
      startTime = getNanoTime();
      stopTime = startTime;
      offset = 0.0;
    }

    void Timer::pause() {
      if (isTimerRunning) {
        stopTime = getNanoTime();
        isTimerRunning = false;
      }
    }

    void Timer::unpause() {
      if (!isTimerRunning) {
        offset += getNanoTime() - stopTime;
        isTimerRunning = true;
      }
    }
  }
}

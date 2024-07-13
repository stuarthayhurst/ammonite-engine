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

    //Creates a new, running timer
    Timer::Timer() {
      isTimerRunning = true;
      startTime = getNanoTime();
      stopTime = 0.0;
      offset = 0.0;
    }

    //Returns the length of time the timer has been active
    double Timer::getTime() {
      //Return the time since last start / stop point, including the offset
      double runTime = isTimerRunning ? getNanoTime() : stopTime;
      return (runTime - startTime - offset) / 1000000000;
    }

    //Set the active time on the timer, without changing pause state
    void Timer::setTime(double newTime) {
      stopTime = getNanoTime();
      startTime = stopTime - (newTime * 1000000000);
      offset = 0.0;
    }

    //Returns whether the timer is running or not
    bool Timer::isRunning() {
      return isTimerRunning;
    }

    //Reset the active time to 0, without changing pause state
    void Timer::reset() {
      startTime = getNanoTime();
      stopTime = startTime;
      offset = 0.0;
    }

    //Pause the timer
    void Timer::pause() {
      if (isTimerRunning) {
        stopTime = getNanoTime();
        isTimerRunning = false;
      }
    }

    //Unpause the timer
    void Timer::unpause() {
      if (!isTimerRunning) {
        offset += getNanoTime() - stopTime;
        isTimerRunning = true;
      }
    }
  }
}

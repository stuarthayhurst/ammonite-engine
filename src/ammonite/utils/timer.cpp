#include <chrono>
#include <ctime>

#include "timer.hpp"

namespace ammonite {
  namespace utils {
    //Creates a new, running timer
    Timer::Timer() {
      this->startTime = std::chrono::steady_clock::now();
      this->setOffset = std::chrono::seconds(0);
      this->pauseOffset = std::chrono::seconds(0);
    }

    //Creates a new, optionally running timer
    Timer::Timer(bool startRunning) {
      this->startTime = std::chrono::steady_clock::now();
      this->setOffset = std::chrono::seconds(0);
      this->pauseOffset = std::chrono::seconds(0);

      if (!startRunning) {
        this->pause();
        this->reset();
      }
    }

    //Writes the length of time the timer has been active
    void Timer::getTime(std::time_t* seconds, std::time_t* nanoseconds) const {
      std::chrono::time_point<std::chrono::steady_clock> now;

      if (this->timerRunning) {
        now = std::chrono::steady_clock::now();
      } else {
        now = stopTime;
      }

      /*
       - Find the length of time between setting the timer and the measurement point
       - Apply a positive offset to handle the time it was initialised to
       - Apply a negative offset to handle paused durations
         - This offset can't grow faster than stopTime or real time
      */
      auto deltaTime = (now - this->startTime) + this->setOffset - this->pauseOffset;

      //Convert the time to seconds and nanoseconds
      const auto deltaTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(deltaTime);
      deltaTime -= deltaTimeSeconds;
      const auto remainderNano = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaTime);

      //Write out times
      *seconds = deltaTimeSeconds.count();
      *nanoseconds = remainderNano.count();
    }

    //Returns the length of time the timer has been active
    double Timer::getTime() const {
      std::time_t seconds = 0;
      std::time_t nanoseconds = 0;

      //Get the time and convert it to a double
      this->getTime(&seconds, &nanoseconds);
      return (double)seconds + ((double)nanoseconds / 1000000000.0);
    }

    //Set the active time on the timer, without changing pause state
    void Timer::setTime(std::time_t seconds, std::time_t nanoseconds) {
      const std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();

      //Pretend the timer was just stopped right now
      if (!this->timerRunning) {
        this->stopTime = now;
      }

      //Pretend the timer was just started right now
      this->startTime = now;

      //Apply the target time as a positive offset
      this->setOffset = std::chrono::seconds(seconds) + std::chrono::nanoseconds(nanoseconds);
      this->pauseOffset = std::chrono::seconds(0);
    }

    //Set the active time on the timer, without changing pause state
    void Timer::setTime(double newTime) {
      const std::time_t seconds = (std::time_t)newTime;
      const std::time_t nanoseconds = (std::time_t)((newTime - (double)seconds) * 1000000000.0);

      //Set the time using seconds and nanoseconds
      this->setTime(seconds, nanoseconds);
    }

    //Returns whether the timer is running or not
    bool Timer::isRunning() const {
      return this->timerRunning;
    }

    //Reset the active time to 0, without changing pause state
    void Timer::reset() {
      this->setTime(0, 0);
    }

    //Pause the timer
    void Timer::pause() {
      if (!this->timerRunning) {
        return;
      }

      //Record the time it stopped
      this->stopTime = std::chrono::steady_clock::now();
      this->timerRunning = false;
    }

    //Unpause the timer
    void Timer::unpause() {
      if (this->timerRunning) {
        return;
      }

      //Record the length of time it was paused for
      this->pauseOffset += std::chrono::steady_clock::now() - this->stopTime;
      this->timerRunning = true;
    }
  }
}

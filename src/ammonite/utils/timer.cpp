#include <ctime>
#include <typeinfo>

#include "timer.hpp"

static_assert(typeid(std::time_t) == typeid(signed long), "expected time_t to be signed");

namespace ammonite {
  namespace utils {
    namespace {
      //Add nanoseconds to *time, account for wrap-around
      void addNanoseconds(std::timespec* time, std::time_t nanoseconds) {
        static_assert(typeid(time->tv_nsec) == typeid(signed long),
                      "expected tv_nsec to be signed");

        //Handle negative nanoseconds
        time->tv_nsec += nanoseconds;
        while (time->tv_nsec < 0) {
          time->tv_nsec += 1000000000;
          time->tv_sec -= 1;
        }

        //Handle more than 1 billion nanoseconds
        while (time->tv_nsec >= 1000000000) {
          time->tv_nsec -= 1000000000;
          time->tv_sec += 1;
        }
      }
    }

    //Creates a new, running timer
    Timer::Timer() {
      std::timespec_get(&startTime, TIME_UTC);
    }

    //Writes the length of time the timer has been active
    void Timer::getTime(std::time_t* seconds, std::time_t* nanoseconds) {
      std::timespec now;

      if (timerRunning) {
        std::timespec_get(&now, TIME_UTC);
      } else {
        now = stopTime;
      }

      //Handle seconds and wrap around nanoseconds
      now.tv_sec -= startTime.tv_sec;
      addNanoseconds(&now, -startTime.tv_nsec);

      //Write out times
      *seconds = now.tv_sec;
      *nanoseconds = now.tv_nsec;
    }

    //Returns the length of time the timer has been active
    double Timer::getTime() {
      std::time_t seconds = 0;
      std::time_t nanoseconds = 0;

      //Get the time and convert it to a double
      getTime(&seconds, &nanoseconds);
      return (double)seconds + ((double)nanoseconds / 1000000000.0);
    }

    //Set the active time on the timer, without changing pause state
    void Timer::setTime(std::time_t seconds, std::time_t nanoseconds) {
      std::timespec now;
      std::timespec_get(&now, TIME_UTC);

      //Pretend the timer was just stopped right now
      if (!timerRunning) {
        stopTime = now;
      }

      //Handle seconds and wrap around nanoseconds
      now.tv_sec -= seconds;
      addNanoseconds(&now, -nanoseconds);

      //Save adjusted start time
      startTime = now;
    }

    //Set the active time on the timer, without changing pause state
    void Timer::setTime(double newTime) {
      const std::time_t seconds = (std::time_t)newTime;
      const std::time_t nanoseconds = (std::time_t)((newTime - (double)seconds) * 1000000000.0);

      //Set the time using seconds and nanoseconds
      setTime(seconds, nanoseconds);
    }

    //Returns whether the timer is running or not
    bool Timer::isRunning() const {
      return timerRunning;
    }

    //Reset the active time to 0, without changing pause state
    void Timer::reset() {
      std::timespec_get(&startTime, TIME_UTC);
      if (!timerRunning) {
        stopTime = startTime;
      }
    }

    //Pause the timer
    void Timer::pause() {
      if (!timerRunning) {
        return;
      }

      std::timespec_get(&stopTime, TIME_UTC);
      timerRunning = false;
    }

    //Unpause the timer
    void Timer::unpause() {
      if (timerRunning) {
        return;
      }

      std::timespec now;
      std::timespec_get(&now, TIME_UTC);

      //Handle seconds and wrap around nanoseconds
      startTime.tv_sec += now.tv_sec - stopTime.tv_sec;
      const std::time_t nDiff = now.tv_nsec - stopTime.tv_nsec;
      addNanoseconds(&startTime, nDiff);

      timerRunning = true;
    }
  }
}

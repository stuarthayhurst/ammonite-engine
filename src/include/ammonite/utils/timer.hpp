#ifndef AMMONITETIMER
#define AMMONITETIMER

#include <chrono>
#include <ctime>

#include "../visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
  namespace utils {
    class Timer {
    private:
      bool timerRunning = true;
      std::chrono::time_point<std::chrono::steady_clock> startTime;
      std::chrono::time_point<std::chrono::steady_clock> stopTime;
      std::chrono::steady_clock::duration setOffset;
      std::chrono::steady_clock::duration pauseOffset;

    public:
      Timer();
      Timer(bool startRunning);
      void getTime(std::time_t* seconds, std::time_t* nanoseconds) const;
      double getTime() const;
      void setTime(std::time_t seconds, std::time_t nanoseconds);
      void setTime(double newTime);
      bool isRunning() const;
      void reset();
      void pause();
      void unpause();
    };
  }
}

#endif

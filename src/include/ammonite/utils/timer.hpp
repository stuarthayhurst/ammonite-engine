#ifndef AMMONITETIMER
#define AMMONITETIMER

#include <ctime>

#include "../visibility.hpp"

namespace AMMONITE_EXPOSED ammonite {
  namespace utils {
    class Timer {
    private:
      bool timerRunning = true;
      std::timespec startTime;
      std::timespec stopTime;

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

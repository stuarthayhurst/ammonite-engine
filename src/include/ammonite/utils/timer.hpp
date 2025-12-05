#ifndef AMMONITETIMER
#define AMMONITETIMER

#include <ctime>

#include "../exposed.hpp"

namespace AMMONITE_EXPOSED ammonite {
  namespace utils {
    class Timer {
    private:
      bool timerRunning = true;
      std::timespec startTime;
      std::timespec stopTime;

    public:
      Timer();
      void getTime(std::time_t* seconds, std::time_t* nanoseconds);
      double getTime();
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

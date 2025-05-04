#ifndef AMMONITETIMER
#define AMMONITETIMER

#include <ctime>

namespace ammonite {
  namespace utils {
    class Timer {
    private:
      bool timerRunning;
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

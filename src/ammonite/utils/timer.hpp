#ifndef TIMER
#define TIMER

namespace ammonite {
  namespace utils {
    class Timer {
    private:
      bool isTimerRunning;
      double startTime;
      double stopTime;
      double offset;

      static double getNanoTime();
      static double getTimeDelta(double timePoint);

    public:
      Timer();
      double getTime();
      void setTime(double newTime);
      bool isRunning();
      void reset();
      void pause();
      void unpause();
    };
  }
}

#endif

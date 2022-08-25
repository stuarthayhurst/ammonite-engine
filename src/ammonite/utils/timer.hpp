#ifndef TIMER
#define TIMER

#include <chrono>

namespace ammonite {
  namespace utils {
    class Timer {
    private:
      bool isTimerRunning = true;
      double startTime = getNanoTime();
      double stopTime = 0.0;
      double offset = 0.0;

      static double getNanoTime() {
        auto systemTime = std::chrono::system_clock::now().time_since_epoch();
        auto systemTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>(systemTime).count();
        return systemTimeNano;
      }

      static double getTimeDelta(double timePoint) {
        return getNanoTime() - timePoint;
      }

    public:
      double getTime() {
        double runTime;
        if (isTimerRunning) {
          runTime = getNanoTime() - startTime - offset;
        } else {
          //If the timer hasn't been unpaused yet, correct for the time
          runTime = stopTime - startTime - offset;
        }

        return runTime / 1000000000;
      }

      void setTime(double newTime) {
        double currentTime = getNanoTime();
        startTime = currentTime - (newTime * 1000000000);
        stopTime = currentTime;
        offset = 0.0;
      }

      bool isRunning() {
        return isTimerRunning;
      }

      void reset() {
        startTime = getNanoTime();
        stopTime = startTime;
        offset = 0.0;
      }

      void pause() {
        if (isTimerRunning) {
          stopTime = getNanoTime();
          isTimerRunning = false;
        }
      }

      void unpause() {
        if (!isTimerRunning) {
          offset += getNanoTime() - stopTime;
          isTimerRunning = true;
        }
      }
    };
  }
}

#endif

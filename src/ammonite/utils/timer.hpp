#ifndef TIMER
#define TIMER

#include <chrono>

namespace ammonite {
  namespace utils {
    namespace {
      static double getNanoTime() {
        auto systemTime = std::chrono::system_clock::now().time_since_epoch();
        auto systemTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>(systemTime).count();
        return systemTimeNano;
      }
    }

    class Timer {
    private:
      static double getTimeDelta() {
        static const double initTime = getNanoTime();
        double systemTime = getNanoTime() - initTime;

        //Convert nanoseconds into seconds
        return systemTime / 1000000000;
      }

      bool isTimerRunning = true;
      double startTime = getTimeDelta();
      double stopTime = 0.0;
      double offset = 0.0;

    public:
      double getTime() {
        if (isTimerRunning) {
          return getTimeDelta() - startTime - offset;
        } else {
          //If the timer hasn't been unpaused yet, correct for the time
          return stopTime - startTime - offset;
        }
      }

      bool isRunning() {
        return isTimerRunning;
      }

      void reset() {
        startTime = getTimeDelta();
        stopTime = 0.0;
        offset = 0.0;
      }

      void pause() {
        if (isTimerRunning) {
          stopTime = getTimeDelta();
          isTimerRunning = false;
        }
      }

      void unpause() {
        if (!isTimerRunning) {
          offset += getTimeDelta() - stopTime;
          isTimerRunning = true;
        }
      }
    };
  }
}

#endif

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

      static double getTimeDelta() {
        static const double initTime = getNanoTime();
        double systemTime = getNanoTime() - initTime;

        //Convert nanoseconds into seconds
        return systemTime / 1000000000;
      }
    }

    class Timer {
      public:
        double getTime() {
          if (running) {
            return getTimeDelta() - start - offset;
          } else {
            //If the timer hasn't been unpaused yet, correct for the time
            return stopTime - start - offset;
          }
        }

        void reset() {
          running = true;
          start = getTimeDelta();
          stopTime = 0.0;
          offset = 0.0;
        }

        void pause() {
          if (running) {
            stopTime = getTimeDelta();
            running = false;
          }
        }

        void unpause() {
          if (!running) {
            offset += getTimeDelta() - stopTime;
            running = true;
          }
        }

      private:
        bool running = true;
        double start = getTimeDelta();
        double stopTime = 0.0;
        double offset = 0.0;
    };
  }
}

#endif

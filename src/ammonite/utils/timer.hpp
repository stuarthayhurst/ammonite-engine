#ifndef TIMER
#define TIMER

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace utils {
    class Timer {
      public:
        double getTime() {
          if (running) {
            return glfwGetTime() - start - offset;
          } else {
            //If the timer hasn't been unpaused yet, correct for the time
            return glfwGetTime() - start - offset - (glfwGetTime() - stopTime);
          }
        }
        void reset() {
          running = true;
          start = glfwGetTime();
          stopTime = 0;
          offset = 0;
        }

        void pause() {
          if (running) {
            stopTime = glfwGetTime();
            running = false;
          }
        }
        void unpause() {
          if (!running) {
            offset += glfwGetTime() - stopTime;
            running = true;
          }
        }
      private:
        bool running = true;
        double start = glfwGetTime();
        double stopTime = 0;
        double offset = 0;
    };
  }
}

#endif

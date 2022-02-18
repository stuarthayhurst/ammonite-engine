#ifndef TIMER
#define TIMER

#include <GLFW/glfw3.h>

namespace timer {
  class timer {
    public:
      double getTime() {
        if (running) {
          return glfwGetTime() - start - offset;
        } else {
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

#endif

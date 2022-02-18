#ifndef TIMER
#define TIMER

#include <GLFW/glfw3.h>

namespace timer {
  class timer {
    public:
      double getTime() {
        return glfwGetTime() - start;
      }
      void reset() {
        start = glfwGetTime();
      }
    private:
      double start = glfwGetTime();
  };
}

#endif

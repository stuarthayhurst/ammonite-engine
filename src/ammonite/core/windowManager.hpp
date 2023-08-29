#ifndef WINDOWMANAGER
#define WINDOWMANAGER

#include <GLFW/glfw3.h>

#include "../constants.hpp"

namespace ammonite {
  namespace window {
    namespace internal {
      int setupGlfw();
      int setupGlew();
      void setupGlfwInput();

      void setContextType(AmmoniteEnum contextType);
      GLFWwindow* createWindow(int width, int height, const char* title);
      GLFWwindow* getWindowPtr();

      void destroyGlfw();
    }
  }
}

#endif

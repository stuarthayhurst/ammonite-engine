#ifndef WINDOWMANAGER
#define WINDOWMANAGER

#include <GLFW/glfw3.h>

#include "../enums.hpp"

namespace ammonite {
  namespace window {
    namespace internal {
      int setupGlfw();
      int setupGlew();
      void setupGlfwInput();
      void destroyGlfw();

      void setContextType(AmmoniteEnum contextType);
      GLFWwindow* createWindow(int width, int height, const char* title);
      GLFWwindow* getWindowPtr();

      void setWindowGeometry(int width, int height, int xPos, int yPos, bool useDecoratedPos);
      void getWindowGeometry(int* width, int* height, int* xPos, int* yPos, bool useDecoratedPos);

      void setFullscreenMonitor(GLFWmonitor* monitor);
      void setFullscreen(bool shouldFullscreen);
      GLFWmonitor* getCurrentMonitor();
      bool getFullscreen();

      int getWidth();
      int getHeight();
      float getAspectRatio();
    }
  }
}

#endif

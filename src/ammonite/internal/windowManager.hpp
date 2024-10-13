#ifndef WINDOWMANAGER
#define WINDOWMANAGER

#include <string>

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
      GLFWwindow* createWindow(unsigned int width, unsigned int height, std::string title);
      GLFWwindow* getWindowPtr();

      void setWindowGeometry(unsigned int width, unsigned int height, unsigned int xPos,
                             unsigned int yPos, bool useDecoratedPos);
      void getWindowGeometry(unsigned int* width, unsigned int* height, unsigned int* xPos,
                             unsigned int* yPos, bool useDecoratedPos);

      void setFullscreenMonitor(GLFWmonitor* monitor);
      void setFullscreen(bool shouldFullscreen);
      GLFWmonitor* getCurrentMonitor();
      bool getFullscreen();

      unsigned int getWidth();
      unsigned int getHeight();
      float getAspectRatio();
    }
  }
}

#endif

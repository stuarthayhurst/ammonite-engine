#ifndef INTERNALWINDOW
#define INTERNALWINDOW

#include <string>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "../enums.hpp"
#include "../internal.hpp"

//Include public interface
#include "../../include/ammonite/window/window.hpp" // IWYU pragma: export

namespace ammonite {
  namespace window {
    namespace AMMONITE_INTERNAL internal {
      struct ImageData {
        unsigned char* data;
        unsigned int width;
        unsigned int height;
      };

      //Implemented by window.hpp
      GLFWwindow* getWindowPtr();
      void setContextType(AmmoniteEnum contextType);

      //Implemented by glfwWindow.hpp
      bool setupGlfw(AmmoniteEnum contextType);
      void destroyGlfw();
      void setFocusCallback(GLFWwindow* windowPtr);

      GLFWwindow* createWindow(unsigned int width, unsigned int height);
      void setWindowResizable(GLFWwindow* windowPtr, bool resizable);
      bool getWindowResizable(GLFWwindow* windowPtr);
      void setTitle(GLFWwindow* windowPtr, const std::string& title);
      void setIcons(GLFWwindow* windowPtr, ImageData* iconData, unsigned int iconCount);
      void setWindowGeometry(GLFWwindow* windowPtr, unsigned int width, unsigned int height,
                             unsigned int xPos, unsigned int yPos, bool useDecoratedSize,
                             bool useDecoratedPos);
      void getWindowGeometry(GLFWwindow* windowPtr, unsigned int* width, unsigned int* height,
                             unsigned int* xPos, unsigned int* yPos, bool useDecoratedSize,
                             bool useDecoratedPos);
      void setFullscreenMonitor(GLFWwindow* windowPtr, GLFWmonitor* monitor);
      void setFullscreen(GLFWwindow* windowPtr, bool shouldFullscreen);
      GLFWmonitor* getCurrentMonitor(GLFWwindow* windowPtr);
      GLFWmonitor** getMonitors(unsigned int* monitorCount);
      bool getFullscreen();
      bool shouldWindowClose(GLFWwindow* windowPtr);

      float getGraphicsAspectRatio();
      unsigned int getGraphicsWidth();
      unsigned int getGraphicsHeight();

      void showFrame(GLFWwindow* windowPtr, bool vsync, float frameLimit);
    }
  }
}

#endif

#ifndef INTERNALWINDOW
#define INTERNALWINDOW

#include <string>

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "../enums.hpp"
#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/window/window.hpp" // IWYU pragma: export

namespace AMMONITE_INTERNAL ammonite {
  namespace window {
    namespace internal {
      struct ImageData {
        unsigned char* data;
        unsigned int width;
        unsigned int height;
      };

      //Implemented by window.hpp
      GLFWwindow* getWindowPtr();
      void setContextType(AmmoniteContextEnum contextType);

      //Implemented by glfwWindow.hpp
      bool setupGlfw(AmmoniteContextEnum contextType);
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
      void setFullscreenMonitor(GLFWwindow* windowPtr, unsigned int monitorIndex);
      void setFullscreen(GLFWwindow* windowPtr, bool shouldFullscreen, unsigned int monitorIndex);
      unsigned int getCurrentMonitorIndex(GLFWwindow* windowPtr);
      unsigned int getMonitorCount();
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

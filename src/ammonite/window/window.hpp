#ifndef WINDOW
#define WINDOW

#include <string>

#include <GLFW/glfw3.h>

#include "../enums.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace window {
    namespace internal {
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
      bool setupGlew();
      void destroyGlfw();
      void setFocusCallback(GLFWwindow* windowPtr);

      GLFWwindow* createWindow(unsigned int width, unsigned int height);
      void setWindowResizable(GLFWwindow* windowPtr, bool resizable);
      bool getWindowResizable(GLFWwindow* windowPtr);
      void setTitle(GLFWwindow* windowPtr, std::string title);
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

    //Exported by the engine
    bool createWindow(unsigned int width, unsigned int height, std::string title);
    bool createWindow(unsigned int width, unsigned int height);
    void destroyWindow();

    void setWindowResizable(bool resizable);
    bool getWindowResizable();

    void setTitle(std::string title);
    void useIcons(std::string* iconPaths, unsigned int iconCount);
    void useIcon(std::string iconPath);
    void useIconDir(std::string iconDirPath);

    void setWindowGeometry(unsigned int width, unsigned int height, unsigned int xPos,
                           unsigned int yPos, bool useDecoratedSize, bool useDecoratedPos);
    void getWindowGeometry(unsigned int* width, unsigned int* height, unsigned int* xPos,
                           unsigned int* yPos, bool useDecoratedSize, bool useDecoratedPos);

    void setFullscreenMonitor(GLFWmonitor* monitor);
    void setFullscreen(bool shouldFullscreen);
    GLFWmonitor* getCurrentMonitor();
    GLFWmonitor** getMonitors(unsigned int* monitorCount);
    bool getFullscreen();

    bool shouldWindowClose();
  }
}

#endif

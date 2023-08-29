#ifndef WINDOW
#define WINDOW

#include <GLFW/glfw3.h>

#include "constants.hpp"

namespace ammonite {
  namespace window {
    GLFWwindow* getWindowPtr();
    void requestContextType(AmmoniteEnum contextType);
    int createWindow(int width, int height, const char* title);
    int createWindow(int width, int height);
    void destroyWindow();

    void setTitle(const char* title);
    void useIconDir(const char* iconDirPath);

    bool getFullscreen();
    int getMonitors(GLFWmonitor*** monitorsPtr);

    void setFullscreenMonitor(GLFWmonitor* monitor);
    void setFullscreen(bool fullscreen);
  }
}

#endif

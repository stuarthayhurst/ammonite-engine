#ifndef WINDOW
#define WINDOW

#include <string>

#include <GLFW/glfw3.h>

#include "enums.hpp"

namespace ammonite {
  namespace window {
    GLFWwindow* getWindowPtr();
    void requestContextType(AmmoniteEnum contextType);
    int createWindow(int width, int height, std::string title);
    int createWindow(int width, int height);
    void destroyWindow();

    bool shouldWindowClose();
    int registerWindowCloseKeybind(int keycode);
    void setTitle(std::string title);
    void useIcons(std::string* iconPaths, unsigned int iconCount);
    void useIcon(std::string iconPath);
    void useIconDir(std::string iconDirPath);

    void setWindowGeometry(int width, int height, int xPos, int yPos, bool useDecoratedPos);
    void getWindowGeometry(int* width, int* height, int* xPos, int* yPos, bool useDecoratedPos);

    void setWindowResizable(bool resizable);
    void getWindowResizable();

    bool getFullscreen();
    GLFWmonitor* getFullscreenMonitor();
    int getMonitors(GLFWmonitor*** monitorsPtr);

    void setFullscreenMonitor(GLFWmonitor* monitor);
    void setFullscreen(bool fullscreen);
  }
}

#endif

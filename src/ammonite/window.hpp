#ifndef WINDOW
#define WINDOW

#include <string>

#include <GLFW/glfw3.h>

#include "enums.hpp"
#include "types.hpp"

namespace ammonite {
  namespace window {
    GLFWwindow* getWindowPtr();
    void requestContextType(AmmoniteEnum contextType);
    bool createWindow(unsigned int width, unsigned int height, std::string title);
    bool createWindow(unsigned int width, unsigned int height);
    void destroyWindow();

    bool shouldWindowClose();
    AmmoniteId registerWindowCloseKeybind(int keycode);
    void setTitle(std::string title);
    void useIcons(std::string* iconPaths, unsigned int iconCount);
    void useIcon(std::string iconPath);
    void useIconDir(std::string iconDirPath);

    void setWindowGeometry(unsigned int width, unsigned int height, unsigned int xPos,
                           unsigned int yPos, bool useDecoratedPos);
    void getWindowGeometry(unsigned int* width, unsigned int* height, unsigned int* xPos,
                           unsigned int* yPos, bool useDecoratedPos);

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

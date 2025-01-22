#ifndef WINDOW
#define WINDOW

#include <string>

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace window {
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

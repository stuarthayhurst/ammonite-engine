#ifndef AMMONITEWINDOW
#define AMMONITEWINDOW

#include <string>

extern "C" {
  #include <GLFW/glfw3.h>
}

namespace ammonite {
  namespace window {
    bool createWindow(unsigned int width, unsigned int height, const std::string& title);
    bool createWindow(unsigned int width, unsigned int height);
    void destroyWindow();

    void setWindowResizable(bool resizable);
    bool getWindowResizable();

    void setTitle(const std::string& title);
    void useIcons(const std::string* iconPaths, unsigned int iconCount);
    void useIcon(const std::string& iconPath);
    void useIconDir(const std::string& iconDirPath);

    void setWindowGeometry(unsigned int width, unsigned int height, unsigned int xPos,
                           unsigned int yPos, bool useDecoratedSize, bool useDecoratedPos);
    void getWindowGeometry(unsigned int* width, unsigned int* height, unsigned int* xPos,
                           unsigned int* yPos, bool useDecoratedSize, bool useDecoratedPos);

    void setFullscreen(bool shouldFullscreen, GLFWmonitor* monitor);
    void setFullscreen(bool shouldFullscreen);
    bool getFullscreen();

    GLFWmonitor* getCurrentMonitor();
    GLFWmonitor** getMonitors(unsigned int* monitorCount);
    void changeFullscreenMonitor(GLFWmonitor* monitor);

    bool shouldWindowClose();
  }
}

#endif

#ifndef WINDOW
#define WINDOW

#include <GLFW/glfw3.h>

#include "constants.hpp"

namespace ammonite {
  namespace windowManager {
    namespace setup {
      bool setupGlfw(int antialiasing);
      bool setupGlew(GLFWwindow* window);
      void setupGlfwInput(GLFWwindow* window);
      void destroyGlfw();
    }

    void requestContextType(AmmoniteEnum contextType);

    GLFWwindow* createWindow(int width, int height, const char* title);
    GLFWwindow* createWindow(int width, int height);

    GLFWwindow* getWindowPtr();

    void setTitle(const char* title);
    void useIconDir(const char* iconDirPath);

    GLFWwindow* setupWindow(int width, int height, const char* title);
    GLFWwindow* setupWindow(int width, int height);
  }
}

#endif

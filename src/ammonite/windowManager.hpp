#ifndef WINDOW
#define WINDOW

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace windowManager {
    namespace setup {
      bool setupGlfw(int antialiasing);
      bool setupGlew(GLFWwindow* window);
      void setupGlfwInput(GLFWwindow* window);
      void destroyGlfw();
    }

    GLFWwindow* createWindow(int width, int height, const char* title);
    GLFWwindow* createWindow(int width, int height);

    void setTitle(GLFWwindow* window, const char* title);
    void useIconDir(GLFWwindow* window, const char* iconDirPath);

    GLFWwindow* setupWindow(int width, int height, int antialiasing, const char* title);
    GLFWwindow* setupWindow(int width, int height, int antialiasing);
  }
}

#endif

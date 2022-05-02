#ifndef WINDOW
#define WINDOW

#include <tuple>
#include <GLFW/glfw3.h>

namespace ammonite {
  namespace windowManager {
    namespace settings {
      void useVsync(bool enabled);
      bool isVsyncEnabled();
    }

    namespace setup {
      bool setupGlfw(int antialiasing);
      bool setupGlew(GLFWwindow* window);
      void setupGlfwInput(GLFWwindow* window);
      void destroyGlfw();
    }

    std::tuple<GLFWwindow*, int*, int*, float*> createWindow(int width, int height);
    void setTitle(GLFWwindow* window, const char* title);
    void useIconDir(GLFWwindow* window, const char* iconDirPath);

    //Wrapper for setup methods and createWindow()
    std::tuple<GLFWwindow*, int*, int*, float*> setupWindow(int newWidth, int newHeight, int antialiasing, const char* title);
  }
}

#endif

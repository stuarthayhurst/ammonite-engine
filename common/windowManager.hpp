#ifndef WINDOW
#define WINDOW

namespace windowManager {
  namespace settings {
    void useVsync(bool enabled);
  }

  namespace setup {
    int setupGlfw(int antialiasing, float openglVersion);
    int setupGlew(GLFWwindow* window);
    void setupGlfwInput(GLFWwindow* window);
  }

  std::tuple<GLFWwindow*, int*, int*, float*> createWindow(int width, int height, const char title[]);
}

#endif

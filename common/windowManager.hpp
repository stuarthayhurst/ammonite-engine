#ifndef WINDOW
#define WINDOW

namespace windowManager {
  namespace setup {
    int setupGlfw(int antialiasing, float openglVersion);
    int setupGlew();
    void setupGlfwInput(GLFWwindow* window);
  }

  GLFWwindow* createWindow(int width, int height, const char title[]);
}

#endif

#ifndef RENDERER
#define RENDERER

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace renderer {
    namespace setup {
      void setupRenderer(GLFWwindow* targetWindow, const char* shaderPath, bool* externalSuccess);
    }

    long getTotalFrames();
    double getFrameTime();

    void drawFrame(const int modelIds[], const int modelCount);
  }
}

#endif

#ifndef RENDERER
#define RENDERER

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "modelManager.hpp"

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

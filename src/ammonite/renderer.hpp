#ifndef RENDERER
#define RENDERER

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "modelManager.hpp"

namespace ammonite {
  namespace renderer {
    namespace setup {
      void setupRenderer(GLFWwindow* targetWindow, GLuint targetProgramId, GLuint targetLightId, GLuint targetDepthId, bool* externalSuccess);
      void setupMatrices(glm::mat4* projectionMatrix, glm::mat4* viewMatrix);
    }

    long getTotalFrames();
    double getFrameTime();

    void drawFrame(const int modelIds[], const int modelCount);
  }
}

#endif

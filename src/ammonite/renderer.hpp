#ifndef RENDERER
#define RENDERER

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "modelManager.hpp"

namespace ammonite {
  namespace renderer {
    namespace setup {
      void setupRenderer(GLFWwindow* targetWindow, GLuint targetProgramId);
    }

    void enableWireframe(bool enabled);
    void drawFrame(const int modelIds[], const int modelCount, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
  }
}

#endif

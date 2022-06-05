#ifndef CONTROLS
#define CONTROLS

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace ammonite {
  namespace controls {
    namespace matrix {
      glm::mat4 getViewMatrix();
      glm::mat4 getProjectionMatrix();
    }

    namespace settings {
      void setMovementSpeed(float newMovementSpeed);
      void setMouseSpeed(float newMouseSpeed);
      void setZoomSpeed(float newZoomMultiplier);

      float getMovementSpeed();
      float getMouseSpeed();
      float getZoomSpeed();
    }

    void setupControls(GLFWwindow* window);
    bool shouldWindowClose();
    void processInput();

    glm::vec3 getCameraPosition();
  }
}

#endif

#ifndef CONTROLS
#define CONTROLS

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace utils {
    namespace controls {
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
    }
  }
}

#endif

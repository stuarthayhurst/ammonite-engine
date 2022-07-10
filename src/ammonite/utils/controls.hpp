#ifndef CONTROLS
#define CONTROLS

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace utils {
    namespace controls {
      void setCameraActive(bool active);
      void setControlsActive(bool active);
      void setInputFocus(bool active);

      bool getCameraActive();
      bool getControlsActive();
      bool getInputFocus();

      void setupControls(GLFWwindow* window);
      bool shouldWindowClose();
      void processInput();
    }
  }
}

#endif

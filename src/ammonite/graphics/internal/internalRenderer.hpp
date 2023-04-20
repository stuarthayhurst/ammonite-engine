/* Internally exposed header:
 - Expose rendering helpers to render core
*/

#ifndef INTERNALRENDERER
#define INTERNALRENDERER

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace renderer {
    namespace setup {
      void internalSetupRenderer(GLFWwindow* targetWindow, const char* shaderPath, bool* externalSuccess);
    }

    void internalDrawFrame();
  }
}

#endif

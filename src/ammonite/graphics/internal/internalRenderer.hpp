/* Internally exposed header:
 - Expose rendering helpers to render core
*/

#ifndef INTERNALRENDERER
#define INTERNALRENDERER

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace renderer {
    namespace setup {
      namespace internal {
        void internalSetupRenderer(GLFWwindow* targetWindow, const char* shaderPath, bool* externalSuccess);
        void connectWindow(GLFWwindow* newWindow);

        bool createShaders(const char* shaderPath, bool* externalSuccess);
        bool checkGPUCapabilities(int* failureCount);
        void setupOpenGLObjects();
      }
    }

    namespace internal {
      void internalDrawFrame();
    }
  }
}

#endif

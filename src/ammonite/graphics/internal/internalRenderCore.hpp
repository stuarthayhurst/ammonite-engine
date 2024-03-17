#ifndef INTERNALRENDERERCORE
#define INTERNALRENDERERCORE

/* Internally exposed header:
 - Expose rendering core to render interface
*/

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace renderer {
    namespace setup {
      namespace internal {
        void internalSetupRenderer(GLFWwindow* targetWindow, const char* shaderPath, bool* externalSuccess);
        void connectWindow(GLFWwindow* newWindow);

        bool checkGPUCapabilities(int* failureCount);
        bool createShaders(const char* shaderPath, bool* externalSuccess);
        void setupOpenGLObjects();
        void deleteShaders();
        void destroyOpenGLObjects();
        void deleteModelCache();
      }
    }

    namespace internal {
      void internalDrawFrame();
      void internalDrawLoadingScreen(int loadingScreenId);
    }
  }
}

#endif

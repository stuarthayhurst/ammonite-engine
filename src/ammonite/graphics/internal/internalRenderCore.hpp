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

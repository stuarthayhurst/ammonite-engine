#ifndef INTERNALRENDERER
#define INTERNALRENDERER

#include <string>

#include <GL/glew.h>

#include "../internal.hpp"
#include "../types.hpp"

//Include public interface
#include "../../include/ammonite/graphics/renderer.hpp" // IWYU pragma: export

namespace ammonite {
  namespace renderer {
    namespace setup {
      namespace AMMONITE_INTERNAL internal {
        bool checkGPUCapabilities(unsigned int* failureCount);
        bool createShaders(const std::string& shaderPath);
        void setupOpenGLObjects();
        void deleteShaders();
        void destroyOpenGLObjects();
        void deleteModelCache();
      }
    }

    namespace AMMONITE_INTERNAL internal {
      void internalDrawFrame();
      void internalDrawLoadingScreen(AmmoniteId loadingScreenId);

      void prepareScreen(GLuint framebufferId, unsigned int width,
                         unsigned int height, bool depthTest);
      void setWireframe(bool enabled);
    }
  }
}

#endif

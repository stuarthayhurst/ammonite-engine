#ifndef INTERNALRENDERERHELPER
#define INTERNALRENDERERHELPER

/* Internally exposed header:
 - Expose rendering helpers to render core
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace ammonite {
  namespace renderer {
    namespace internal {
      void prepareScreen(GLuint framebufferId, unsigned int width,
                         unsigned int height, bool depthTest);
      void finishFrame(GLFWwindow* window);
      void setWireframe(bool enabled);
    }
  }
}

#endif

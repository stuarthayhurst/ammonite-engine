extern "C" {
  #include <epoxy/gl.h>
}

#include "renderer.hpp"

/*
 - Generic helpers for the render core
*/

namespace ammonite {
  namespace renderer {
    namespace internal {
      void prepareScreen(GLuint framebufferId, unsigned int width,
                         unsigned int height, bool depthTest) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        if (depthTest) {
          glEnable(GL_DEPTH_TEST);
        } else {
          glDisable(GL_DEPTH_TEST);
        }
      }

      void setWireframe(bool enabled) {
        //Avoid unnecessary polygon mode updates
        static bool oldEnabled = !enabled;
        if (oldEnabled == enabled) {
          return;
        }

        //Change the draw mode
        glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
        oldEnabled = enabled;
      }
    }
  }
}

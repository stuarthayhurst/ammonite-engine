#include <GL/glew.h>

/*
 - Generic helpers for the render core
*/

namespace ammonite {
  namespace renderer {
    namespace internal {
      void prepareScreen(int framebufferId, int width, int height, bool depthTest) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glViewport(0, 0, width, height);
        if (depthTest) {
          glEnable(GL_DEPTH_TEST);
        } else {
          glDisable(GL_DEPTH_TEST);
        }
      }

      void setWireframe(bool enabled) {
        //Avoid changing polygon mode if it's already correct
        static bool oldEnabled = false;
        if (oldEnabled == enabled) {
          return;
        }
        oldEnabled = enabled;

        //Change the draw mode
        glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
      }
    }
  }
}

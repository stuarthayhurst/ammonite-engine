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
    }
  }
}

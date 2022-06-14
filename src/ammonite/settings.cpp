#include <GLFW/glfw3.h>

namespace ammonite {
  namespace settings {
    namespace graphics {
      namespace {
        struct GraphicsSettings {
          bool vsyncEnabled = true;
          float frameTarget = 0.0f;
        } graphics;
      }

      void setVsync(bool enabled) {
        glfwSwapInterval(int(enabled));
        graphics.vsyncEnabled = enabled;
      }

      bool getVsync() {
        return graphics.vsyncEnabled;
      }

      void setFrameTarget(float frameTarget) {
        graphics.frameTarget = frameTarget;
      }

      float getFrameTarget() {
        return graphics.frameTarget;
      }
    }
  }
}

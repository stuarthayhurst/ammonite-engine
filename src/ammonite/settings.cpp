#include <GLFW/glfw3.h>

namespace ammonite {
  namespace settings {
    namespace graphics {
      namespace {
        struct GraphicsSettings {
          bool vsyncEnabled = true;
          float frameLimit = 0.0f;
          int shadowRes = 1024;
        } graphics;
      }

      void setVsync(bool enabled) {
        glfwSwapInterval(int(enabled));
        graphics.vsyncEnabled = enabled;
      }

      bool getVsync() {
        return graphics.vsyncEnabled;
      }

      void setFrameLimit(float frameLimit) {
        graphics.frameLimit = frameLimit;
      }

      float getFrameLimit() {
        return graphics.frameLimit;
      }

      void setShadowRes(int shadowRes) {
        graphics.shadowRes = shadowRes;
      }

      int getShadowRes() {
        return graphics.shadowRes;
      }
    }
  }
}

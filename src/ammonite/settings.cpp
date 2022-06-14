#include <GLFW/glfw3.h>

namespace ammonite {
  namespace settings {
    namespace graphics {
      namespace {
        struct GraphicsSettings {
          bool vsyncEnabled = true;
        } graphics;
      }

      void setVsync(bool enabled) {
        glfwSwapInterval(int(enabled));
        graphics.vsyncEnabled = enabled;
      }

      bool getVsync() {
        return graphics.vsyncEnabled;
      }
    }
  }
}

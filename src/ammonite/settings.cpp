#include <GLFW/glfw3.h>

namespace ammonite {
  namespace settings {
    namespace graphics {
      namespace {
        bool vsyncEnabled = true;
      }

      void setVsync(bool enabled) {
        glfwSwapInterval(int(enabled));
        vsyncEnabled = enabled;
      }

      bool getVsync() {
        return vsyncEnabled;
      }
    }
  }
}

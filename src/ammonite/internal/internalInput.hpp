#ifndef INTERNALINPUT
#define INTERNALINPUT

/* Internally exposed header:
 - Expose function to setup window focus callback
*/

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace input {
    namespace internal {
      void setupFocusCallback(GLFWwindow* windowPtr);
    }
  }
}

#endif

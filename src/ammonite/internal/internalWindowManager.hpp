/* Internally exposed header:
 - Expose window pointer
*/

#ifndef INTERNALWINDOW
#define INTERNALWINDOW

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace windowManager {
    namespace internal {
      GLFWwindow* getWindowPtr();
    }
  }
}

#endif

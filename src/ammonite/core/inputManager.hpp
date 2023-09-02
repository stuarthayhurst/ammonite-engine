#ifndef INPUTMANAGER
#define INPUTMANAGER

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace input {
    namespace internal {
      void setupCallback(GLFWwindow* windowPtr);
      int registerRawKeybind(int keycode, bool toggle,
                             void(*callback)(int, int, void*), void* userPtr);
      void unregisterKeybind(int keycode);
    }
  }
}

#endif
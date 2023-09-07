#ifndef INPUTMANAGER
#define INPUTMANAGER

#include <GLFW/glfw3.h>

namespace ammonite {
  namespace input {
    namespace internal {
      void setupInputCallback(GLFWwindow* windowPtr);
      void runCallbacks();

      void setInputBlock(bool inputBlocked);
      bool getInputBlock();

      int registerRawKeybind(int keycode, bool allowOverride, bool toggle,
                             void(*callback)(int, int, void*), void* userPtr);
      int unregisterKeybind(int keycode);
      bool isKeybindRegistered(int keycode);
      void moveKeybindData(int oldKeybind, int newKeybind);
    }
  }
}

#endif

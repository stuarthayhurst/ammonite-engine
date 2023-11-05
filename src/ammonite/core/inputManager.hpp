#ifndef INPUTMANAGER
#define INPUTMANAGER

#include <vector>

#include <GLFW/glfw3.h>

#include "../constants.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      void setupInputCallback(GLFWwindow* windowPtr);
      void runCallbacks();

      void setInputBlock(bool inputBlocked);
      bool getInputBlock();

      int registerRawKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                             bool toggle, void(*callback)(std::vector<int>, int, void*),
                             void* userPtr);
      int unregisterKeybind(int keybindId);
      bool isKeycodeRegistered(int keycode);
      int changeKeybindKeycodes(int keybindId, int newKeycodes[], int count);
    }
  }
}

#endif

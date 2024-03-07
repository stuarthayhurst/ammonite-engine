#ifndef INPUTMANAGER
#define INPUTMANAGER

#include <GLFW/glfw3.h>

#include "../enums.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      void setupInputCallback(GLFWwindow* windowPtr);
      void runCallbacks();

      void setInputBlock(bool inputBlocked);
      bool getInputBlock();
      bool* getInputBlockPtr();

      int registerRawKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                             bool toggle, AmmoniteKeyCallback callback, void* userPtr);
      int unregisterKeybind(int keybindId);
      bool isKeycodeRegistered(int keycodes[], int count);
      int changeKeybindKeycodes(int keybindId, int newKeycodes[], int count);
    }
  }
}

#endif

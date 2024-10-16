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

      AmmoniteId registerRawKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                                    bool toggle, AmmoniteKeyCallback callback, void* userPtr);
      bool unregisterKeybind(AmmoniteId keybindId);
      bool isKeycodeRegistered(int keycodes[], int count);
      bool changeKeybindKeycodes(AmmoniteId keybindId, int newKeycodes[], int count);
    }
  }
}

#endif

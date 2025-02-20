#ifndef INTERNALINPUT
#define INTERNALINPUT

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "internal.hpp"

#include "enums.hpp"
#include "types.hpp"

//Include public interface
#include "../include/ammonite/input.hpp"

namespace ammonite {
  namespace input {
    namespace AMMONITE_INTERNAL internal {
      //Implemented by internal/inputManager.hpp
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

#ifndef INTERNALINPUT
#define INTERNALINPUT

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "../enums.hpp"
#include "../internal.hpp"
#include "../types.hpp"

//Include public interface
#include "../../include/ammonite/input/input.hpp" // IWYU pragma: export

namespace ammonite {
  namespace input {
    namespace AMMONITE_INTERNAL internal {
      //Implemented by inputManager.cpp
      void setupInputCallback(GLFWwindow* windowPtr);

      void setInputBlock(bool inputBlocked);
      bool getInputBlock();

      void runCallbacks();
      AmmoniteId registerRawKeybind(const int keycodes[], int count, AmmoniteEnum overrideMode,
                                    bool toggle, AmmoniteKeyCallback callback, void* userPtr);
      bool unregisterKeybind(AmmoniteId keybindId);
      bool isKeycodeRegistered(const int keycodes[], int count);
      bool changeKeybindKeycodes(AmmoniteId keybindId, const int newKeycodes[], int count);
    }
  }
}

#endif

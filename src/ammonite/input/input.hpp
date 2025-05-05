#ifndef INTERNALINPUT
#define INTERNALINPUT

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "../internal.hpp"
#include "../utils/id.hpp"

//Include public interface
#include "../../include/ammonite/input/input.hpp" // IWYU pragma: export

namespace ammonite {
  namespace input {
    namespace AMMONITE_INTERNAL internal {
      //Implemented by keyInputManager.cpp
      void setupInputCallback(GLFWwindow* windowPtr);

      void setInputBlock(bool inputBlocked);
      bool getInputBlock();

      void runCallbacks();
      AmmoniteId registerRawKeybind(const int keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                                    bool toggle, AmmoniteKeyCallback callback, void* userPtr);
      bool unregisterKeybind(AmmoniteId keybindId);
      bool isKeycodeRegistered(const int keycodes[], int count);
      bool changeKeybindKeycodes(AmmoniteId keybindId, const int newKeycodes[], int count);

      //Implemented by mouseInputManager.cpp
      void setupMouseCallback(GLFWwindow* windowPtr);

      void setCursorPositionCallback(AmmoniteCursorCallback callback, void* userPtr);
      void setMouseButtonCallback(AmmoniteButtonCallback callback, void* userPtr);
      void setScrollWheelCallback(AmmoniteScrollCallback callback, void* userPtr);
    }
  }
}

#endif

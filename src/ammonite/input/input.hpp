#ifndef INTERNALINPUT
#define INTERNALINPUT

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "keycodes.hpp"

#include "../internal.hpp"
#include "../utils/id.hpp"

//Include public interface
#include "../../include/ammonite/input/input.hpp" // IWYU pragma: export

namespace ammonite {
  namespace input {
    namespace AMMONITE_INTERNAL internal {
      //Implemented by glfwKeyInput.cpp
      void setupInputCallback(GLFWwindow* windowPtr);
      KeyStateEnum getKeyState(AmmoniteKeycode keycode);
      void updateEvents();

      //Implemented by keyInputManager.cpp
      KeyStateEnum* getKeycodeStatePtr(AmmoniteKeycode keycode);

      void setKeyInputBlock(bool inputBlocked);
      bool getKeyInputBlock();

      void runCallbacks();
      AmmoniteId registerRawKeybind(const AmmoniteKeycode keycodes[], int count,
                                    AmmoniteReleaseEnum overrideMode, bool toggle,
                                    AmmoniteKeyCallback callback, void* userPtr);
      bool unregisterKeybind(AmmoniteId keybindId);
      bool isKeycodeRegistered(const AmmoniteKeycode keycodes[], int count);
      bool changeKeybindKeycodes(AmmoniteId keybindId,
                                 const AmmoniteKeycode newKeycodes[], int count);

      //Implemented by glfwMouseInput.cpp
      void setupMouseCallback(GLFWwindow* windowPtr);

      void setMouseInputBlock(bool inputBlocked);
      bool getMouseInputBlock();

      void setCursorPositionCallback(AmmoniteCursorCallback callback, void* userPtr);
      void setMouseButtonCallback(AmmoniteButtonCallback callback, void* userPtr);
      void setScrollWheelCallback(AmmoniteScrollCallback callback, void* userPtr);
    }
  }
}

#endif

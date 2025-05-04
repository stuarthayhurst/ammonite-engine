extern "C" {
  #include <GLFW/glfw3.h>
}

#include "input.hpp"

#include "controls.hpp"

#include "../utils/id.hpp"

#define OVERRIDE_MODE_DEFAULT AMMONITE_FORCE_RELEASE

namespace ammonite {
  namespace input {
    //A: Multi-key, override mode, no toggle
    //Hands off to core
    AmmoniteId registerKeybind(int keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr) {
      return internal::registerRawKeybind(keycodes, count, overrideMode, false, callback, userPtr);
    }

    //B: Multi-key, override mode, toggle
    //Hands off to core
    AmmoniteId registerToggleKeybind(int keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr) {
      return internal::registerRawKeybind(keycodes, count, overrideMode, true, callback, userPtr);
    }

    //C: Multi-key, no override mode, no toggle
    //Hands off to A
    AmmoniteId registerKeybind(int keycodes[], int count,
                        AmmoniteKeyCallback callback, void* userPtr) {
      return registerKeybind(keycodes, count, OVERRIDE_MODE_DEFAULT, callback, userPtr);
    }

    //D: Multi-key, no override mode, toggle
    //Hands off to B
    AmmoniteId registerToggleKeybind(int keycodes[], int count,
                                     AmmoniteKeyCallback callback, void* userPtr) {
      return registerToggleKeybind(keycodes, count, OVERRIDE_MODE_DEFAULT, callback, userPtr);
    }

    //E: Single key, override mode, no toggle
    //Hands off to C
    AmmoniteId registerKeybind(int keycode, AmmoniteReleaseEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr) {
      return registerKeybind(&keycode, 1, overrideMode, callback, userPtr);
    }

    //F: Single key, override mode, toggle
    //Hands off to D
    AmmoniteId registerToggleKeybind(int keycode, AmmoniteReleaseEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr) {
      return registerToggleKeybind(&keycode, 1, overrideMode, callback, userPtr);
    }

    //G: Single key, no override mode, no toggle
    //Hands off to E
    AmmoniteId registerKeybind(int keycode, AmmoniteKeyCallback callback, void* userPtr) {
      return registerKeybind(keycode, OVERRIDE_MODE_DEFAULT, callback, userPtr);
    }

    //H: Single key, no override mode, toggle
    //Hands off to F
    AmmoniteId registerToggleKeybind(int keycode, AmmoniteKeyCallback callback, void* userPtr) {
      return registerToggleKeybind(keycode, OVERRIDE_MODE_DEFAULT, callback, userPtr);
    }

    bool unregisterKeybind(AmmoniteId keybindId) {
      return internal::unregisterKeybind(keybindId);
    }

    //Return true if all keys are at least part of the same combo
    bool isKeycodeRegistered(int keycodes[], int count) {
      return internal::isKeycodeRegistered(keycodes, count);
    }

    //Single key variant of the above
    bool isKeycodeRegistered(int keycode) {
      return isKeycodeRegistered(&keycode, 1);
    }

    void setInputFocus(bool active) {
      internal::setInputBlock(!active);
      ammonite::controls::internal::setCursorFocus(active);
    }

    bool getInputFocus() {
      return !internal::getInputBlock();
    }

    bool changeKeybind(AmmoniteId keybindId, int keycodes[], int count) {
      return internal::changeKeybindKeycodes(keybindId, keycodes, count);
    }

    bool changeKeybind(AmmoniteId keybindId, int keycode) {
      return internal::changeKeybindKeycodes(keybindId, &keycode, 1);
    }

    void updateInput() {
        //Poll GLFW for input, run keybind callbacks
        glfwPollEvents();
        ammonite::input::internal::runCallbacks();
    }
  }
}

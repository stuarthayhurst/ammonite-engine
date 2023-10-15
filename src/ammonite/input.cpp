#include <iostream>

#include "core/inputManager.hpp"

#include "utils/internal/internalControls.hpp"

#include "utils/debug.hpp"
#include "constants.hpp"

#define OVERRIDE_MODE_DEFAULT AMMONITE_FORCE_RELEASE

namespace ammonite {
  namespace input {
    int registerKeybind(int keycode, AmmoniteEnum overrideMode,
                        void(*callback)(int, int, void*), void* userPtr) {
      return internal::registerRawKeybind(keycode, overrideMode, false, callback, userPtr);
    }

    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr) {
      return registerKeybind(keycode, OVERRIDE_MODE_DEFAULT, callback, userPtr);
    }

    int registerToggleKeybind(int keycode, AmmoniteEnum overrideMode,
                              void(*callback)(int, int, void*), void* userPtr) {
      return internal::registerRawKeybind(keycode, overrideMode, true, callback, userPtr);
    }

    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr) {
      return registerToggleKeybind(keycode, OVERRIDE_MODE_DEFAULT, callback, userPtr);
    }

    int unregisterKeybind(int keybindId) {
      return internal::unregisterKeybind(keybindId);
    }

    bool isKeycodeRegistered(int keycode) {
      return internal::isKeycodeRegistered(keycode);
    }

    void setInputFocus(bool active) {
      internal::setInputBlock(!active);
      ammonite::utils::controls::internal::setInputFocus(active);
    }

    bool getInputFocus() {
      return !internal::getInputBlock();
    }

    //Callback and setup function
    namespace internal {
      namespace {
        static void windowFocusCallback(GLFWwindow*, int focused) {
          //Unbind input with window focus (fixes missing mouse)
          if (!focused) {
            setInputFocus(focused);
          }
        }
      }

      void setupFocusCallback(GLFWwindow* windowPtr) {
        //Set callback to update input state on window focus
        glfwSetWindowFocusCallback(windowPtr, windowFocusCallback);
      }
    }
  }
}

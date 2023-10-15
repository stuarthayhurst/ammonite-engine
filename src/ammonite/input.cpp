#include <iostream>

#include "core/inputManager.hpp"
#include "core/engineKeybinds.hpp"

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
      //Fail if attempting to unregister an engine keybind
      if (internal::isKeybindInternal(keybindId)) {
        ammoniteInternalDebug << "Failed to unregister keybind, keybind is internal" << std::endl;
        return -1;
      }

      return internal::unregisterKeybind(keybindId);
    }

    bool isKeycodeRegistered(int keycode) {
      return internal::isKeycodeRegistered(keycode);
    }

    int setEngineKeybind(AmmoniteEnum engineConstant, int keycode) {
      //Check engine constant exists in internal store
      if (!internal::isEngineKeybindValid(engineConstant)) {
        ammoniteInternalDebug << "Failed to register keybind, invalid constant" << std::endl;
        return -1;
      }

      //Check new keycode isn't already registered
      if (internal::isKeycodeRegistered(keycode)) {
        ammoniteInternalDebug << "Failed to register keybind, keycode already registered" << std::endl;
        return -1;
      }

      int existingKeybindId = internal::getExistingKeybindId(engineConstant);
      if (internal::isKeycodeRegistered(existingKeybindId)) {
        //Move old keybind data to new keycode
        internal::moveKeybindData(existingKeybindId, keycode);
      }

      //Update saved keybind
      internal::setEngineKeybind(engineConstant, keycode);

      return 0;
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

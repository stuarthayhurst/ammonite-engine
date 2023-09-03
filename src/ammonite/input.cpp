#include <iostream>

#include "core/inputManager.hpp"
#include "core/engineKeybinds.hpp"

#include "utils/debug.hpp"
#include "constants.hpp"

namespace ammonite {
  namespace input {
    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr) {
      return internal::registerRawKeybind(keycode, false, callback, userPtr);
    }

    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr) {
      return internal::registerRawKeybind(keycode, true, callback, userPtr);
    }

    int unregisterKeybind(int keycode) {
      //Fail if attempting to unregister an engine keybind
      if (internal::isKeybindInternal(keycode)) {
        ammoniteInternalDebug << "Failed to unregister keybind, keybind is internal" << std::endl;
        return -1;
      }

      internal::unregisterKeybind(keycode);
      return 0;
    }

    bool isKeybindRegistered(int keycode) {
      return internal::isKeybindRegistered(keycode);
    }

    int setEngineKeybind(AmmoniteEnum engineConstant, int keycode) {
      //Check engine constant exists in internal store
      if (!internal::isEngineKeybindValid(engineConstant)) {
        ammoniteInternalDebug << "Failed to register keybind, invalid constant" << std::endl;
        return -1;
      }

      //Check new keycode isn't already registered
      if (internal::isKeybindRegistered(keycode)) {
        ammoniteInternalDebug << "Failed to register keybind, keycode already registered" << std::endl;
        return -1;
      }

      int existingKeycode = internal::getExistingKeycode(engineConstant);
      if (internal::isKeybindRegistered(existingKeycode)) {
        //Move old keybind data to new keycode
        internal::moveKeybindData(existingKeycode, keycode);
      }

      //Update saved keybind
      internal::setEngineKeybind(engineConstant, keycode);

      return 0;
    }
  }
}

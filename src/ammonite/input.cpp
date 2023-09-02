#include "core/inputManager.hpp"

namespace ammonite {
  namespace input {
    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr) {
      return internal::registerRawKeybind(keycode, false, callback, userPtr);
    }

    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr) {
      return internal::registerRawKeybind(keycode, true, callback, userPtr);
    }

    void unregisterKeybind(int keycode) {
      internal::unregisterKeybind(keycode);
    }

    bool isKeybindRegistered(int keycode) {
      return internal::isKeybindRegistered(keycode);
    }
  }
}

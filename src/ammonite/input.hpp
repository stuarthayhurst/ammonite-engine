#ifndef INPUT
#define INPUT

#include "constants.hpp"

namespace ammonite {
  namespace input {
    //Callback takes keycode, (allowOverride), action, user pointer
    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    int registerKeybind(int keycode, bool allowOverride,
                        void(*callback)(int, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, bool allowOverride,
                              void(*callback)(int, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    int unregisterKeybind(int keycode);
    bool isKeybindRegistered(int keycode);

    void setInputFocus(bool active);
    bool getInputFocus();

    int setEngineKeybind(AmmoniteEnum engineConstant, int keycode);
  }
}

#endif

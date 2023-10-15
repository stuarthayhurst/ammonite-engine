#ifndef INPUT
#define INPUT

#include "constants.hpp"

namespace ammonite {
  namespace input {
    //Keybind takes keycode, (overrideMode), action callback, user pointer
    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    int registerKeybind(int keycode, AmmoniteEnum overrideMode,
                        void(*callback)(int, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, AmmoniteEnum overrideMode,
                              void(*callback)(int, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    int unregisterKeybind(int keybindId);
    bool isKeycodeRegistered(int keycode);

    void setInputFocus(bool active);
    bool getInputFocus();

    int setEngineKeybind(AmmoniteEnum engineConstant, int keycode);
  }
}

#endif

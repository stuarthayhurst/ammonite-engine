#ifndef INPUT
#define INPUT

#include "constants.hpp"

namespace ammonite {
  namespace input {
    //Callback takes keycode, action, user pointer
    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    void unregisterKeybind(int keycode);
    bool isKeybindRegistered(int keycode);

    int setEngineKeybind(int keycode, AmmoniteEnum engineConstant);
  }
}

#endif

#ifndef INPUT
#define INPUT

namespace ammonite {
  namespace input {
    //Callback takes keycode, action, user pointer
    int registerKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, void(*callback)(int, int, void*), void* userPtr);
    void unregisterKeybind(int keycode);
  }
}

#endif

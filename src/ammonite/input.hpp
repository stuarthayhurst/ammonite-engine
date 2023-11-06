#ifndef INPUT
#define INPUT

#include <vector>

#include "constants.hpp"

namespace ammonite {
  namespace input {
    /*
     - All register(Toggle)Keybind() calls map to registerRawKeybind()
    */

    //Keybind takes keycodes, count, (overrideMode), action callback, user pointer
    int registerKeybind(int keycodes[], int count,
                        void(*callback)(std::vector<int>, int, void*), void* userPtr);
    int registerKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                        void(*callback)(std::vector<int>, int, void*), void* userPtr);
    int registerToggleKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                              void(*callback)(std::vector<int>, int, void*), void* userPtr);
    int registerToggleKeybind(int keycodes[], int count,
                              void(*callback)(std::vector<int>, int, void*), void* userPtr);

    //Single key variants of the above
    int registerKeybind(int keycode, void(*callback)(std::vector<int>, int, void*),
                        void* userPtr);
    int registerKeybind(int keycode, AmmoniteEnum overrideMode,
                        void(*callback)(std::vector<int>, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, AmmoniteEnum overrideMode,
                              void(*callback)(std::vector<int>, int, void*), void* userPtr);
    int registerToggleKeybind(int keycode, void(*callback)(std::vector<int>, int, void*),
                              void* userPtr);

    int unregisterKeybind(int keybindId);
    bool isKeycodeRegistered(int keycodes[], int count);
    bool isKeycodeRegistered(int keycode);

    int changeKeybind(int keybindId, int keycodes[], int count);
    int changeKeybind(int keybindId, int keycode);

    void setInputFocus(bool active);
    bool getInputFocus();
  }
}

#endif

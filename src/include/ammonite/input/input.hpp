#ifndef INPUT
#define INPUT

#include <vector>

#include "../types.hpp"

enum AmmoniteReleaseEnum : unsigned char {
  AMMONITE_ALLOW_OVERRIDE,
  AMMONITE_ALLOW_RELEASE,
  AMMONITE_FORCE_RELEASE,
  AMMONITE_RESPECT_BLOCK
};

enum KeyStateEnum : unsigned char {
  AMMONITE_HELD,
  AMMONITE_RELEASED,
  AMMONITE_REPEAT
};

using AmmoniteKeyCallback = void (*)(const std::vector<int>& keycodes, KeyStateEnum action, void* userPtr);

namespace ammonite {
  namespace input {
    /*
     - All register(Toggle)Keybind() calls map to registerRawKeybind()
    */

    //Keybind takes keycodes, count, (overrideMode), action callback, user pointer
    AmmoniteId registerKeybind(int keycodes[], int count,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerKeybind(int keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycodes[], int count,
                                     AmmoniteKeyCallback callback, void* userPtr);

    //Single key variants of the above
    AmmoniteId registerKeybind(int keycode, AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerKeybind(int keycode, AmmoniteReleaseEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycode, AmmoniteReleaseEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycode, AmmoniteKeyCallback callback, void* userPtr);

    bool unregisterKeybind(AmmoniteId keybindId);
    bool isKeycodeRegistered(int keycodes[], int count);
    bool isKeycodeRegistered(int keycode);

    bool changeKeybind(AmmoniteId keybindId, int keycodes[], int count);
    bool changeKeybind(AmmoniteId keybindId, int keycode);

    void setInputFocus(bool active);
    bool getInputFocus();

    void updateInput();
  }
}

#endif

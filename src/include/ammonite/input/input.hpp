#ifndef AMMONITEINPUT
#define AMMONITEINPUT

#include <vector>

#include "keycodes.hpp"

#include "../utils/id.hpp"
#include "../visibility.hpp"

enum AmmoniteReleaseEnum : unsigned char {
  AMMONITE_ALLOW_OVERRIDE,
  AMMONITE_ALLOW_RELEASE,
  AMMONITE_FORCE_RELEASE,
  AMMONITE_RESPECT_BLOCK
};

enum KeyStateEnum : unsigned char {
  AMMONITE_PRESSED,
  AMMONITE_RELEASED,
  AMMONITE_REPEAT
};

/*
 - Callbacks aren't allowed to modify keybinds
 - If this behaviour is required, add then to a queue and process them after
   ammonite::input::updateInput() is done
*/
using AmmoniteKeyCallback = void (*)(const std::vector<AmmoniteKeycode>& keycodes, KeyStateEnum action, void* userPtr);
using AmmoniteCursorCallback = void (*)(double xPosition, double yPosition, double xDelta, double yDelta, void* userPtr);
using AmmoniteButtonCallback = void (*)(AmmoniteButton button, KeyStateEnum action, void* userPtr);
using AmmoniteScrollCallback = void (*)(double xOffset, double yOffset, void* userPtr);

namespace AMMONITE_EXPOSED ammonite {
  namespace input {
    /*
     - All register(Toggle)Keybind() calls map to registerRawKeybind()
    */

    //Keybind takes keycodes, count, (overrideMode), action callback, user pointer
    AmmoniteId registerKeybind(AmmoniteKeycode keycodes[], int count,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerKeybind(AmmoniteKeycode keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(AmmoniteKeycode keycodes[], int count, AmmoniteReleaseEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(AmmoniteKeycode keycodes[], int count,
                                     AmmoniteKeyCallback callback, void* userPtr);

    //Single key variants of the above
    AmmoniteId registerKeybind(AmmoniteKeycode keycode, AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerKeybind(AmmoniteKeycode keycode, AmmoniteReleaseEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(AmmoniteKeycode keycode, AmmoniteReleaseEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(AmmoniteKeycode keycode, AmmoniteKeyCallback callback, void* userPtr);

    bool unregisterKeybind(AmmoniteId keybindId);
    bool isKeycodeRegistered(AmmoniteKeycode keycodes[], int count);
    bool isKeycodeRegistered(AmmoniteKeycode keycode);

    bool changeKeybind(AmmoniteId keybindId, AmmoniteKeycode keycodes[], int count);
    bool changeKeybind(AmmoniteId keybindId, AmmoniteKeycode keycode);

    void setAnykeyCallback(AmmoniteKeyCallback callback, void* userPtr);

    void setCursorPositionCallback(AmmoniteCursorCallback callback, void* userPtr);
    void setMouseButtonCallback(AmmoniteButtonCallback callback, void* userPtr);
    void setScrollWheelCallback(AmmoniteScrollCallback callback, void* userPtr);

    void setInputFocus(bool active);
    bool getInputFocus();

    void updateInput();
  }
}

#endif

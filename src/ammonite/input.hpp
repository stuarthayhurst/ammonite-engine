#ifndef INTERNALINPUT
#define INTERNALINPUT

extern "C" {
  #include <GLFW/glfw3.h>
}

#include "internal.hpp"

#include "enums.hpp"
#include "types.hpp"

namespace ammonite {
  namespace input {
    namespace AMMONITE_INTERNAL internal {
      //Implemented by internal/inputManager.hpp
      void setupInputCallback(GLFWwindow* windowPtr);
      void runCallbacks();

      void setInputBlock(bool inputBlocked);
      bool getInputBlock();
      bool* getInputBlockPtr();

      AmmoniteId registerRawKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                                    bool toggle, AmmoniteKeyCallback callback, void* userPtr);
      bool unregisterKeybind(AmmoniteId keybindId);
      bool isKeycodeRegistered(int keycodes[], int count);
      bool changeKeybindKeycodes(AmmoniteId keybindId, int newKeycodes[], int count);
    }

    //Exported by the engine

    /*
     - All register(Toggle)Keybind() calls map to registerRawKeybind()
    */

    //Keybind takes keycodes, count, (overrideMode), action callback, user pointer
    AmmoniteId registerKeybind(int keycodes[], int count,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycodes[], int count, AmmoniteEnum overrideMode,
                                     AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycodes[], int count,
                                     AmmoniteKeyCallback callback, void* userPtr);

    //Single key variants of the above
    AmmoniteId registerKeybind(int keycode, AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerKeybind(int keycode, AmmoniteEnum overrideMode,
                               AmmoniteKeyCallback callback, void* userPtr);
    AmmoniteId registerToggleKeybind(int keycode, AmmoniteEnum overrideMode,
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

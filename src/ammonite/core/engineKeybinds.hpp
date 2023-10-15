#ifndef ENGINEKEYBINDS
#define ENGINEKEYBINDS

#include "../constants.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      bool isEngineKeybindValid(AmmoniteEnum engineKeybind);
      bool isKeybindInternal(int keybindId);

      int getExistingKeybind(AmmoniteEnum engineKeybind);
      void setEngineKeybind(AmmoniteEnum engineKeybind, int keycode);
    }
  }
}

#endif

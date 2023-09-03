#ifndef ENGINEKEYBINDS
#define ENGINEKEYBINDS

#include "../constants.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      bool isEngineKeybindValid(AmmoniteEnum engineKeybind);
      bool isKeybindInternal(int keycode);

      int getExistingKeycode(AmmoniteEnum engineKeybind);
      void setEngineKeybind(AmmoniteEnum engineKeybind, int keycode);
    }
  }
}

#endif

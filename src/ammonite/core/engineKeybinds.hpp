#ifndef ENGINEKEYBINDS
#define ENGINEKEYBINDS

#include "../constants.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      bool isEngineKeybindValid(AmmoniteEnum engineKeybind);
      int getExistingKeycode(AmmoniteEnum engineKeybind);
      void setEngineKeybind(int keycode, AmmoniteEnum engineKeybind);
    }
  }
}

#endif

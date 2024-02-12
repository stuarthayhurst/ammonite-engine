#ifndef KEYBINDS
#define KEYBINDS

#include "../enums.hpp"

namespace ammonite {
  namespace utils {
    namespace controls {
      void setKeybind(AmmoniteEnum engineKey, int keycode);
      int getKeybind(AmmoniteEnum engineKey);
    }
  }
}

#endif

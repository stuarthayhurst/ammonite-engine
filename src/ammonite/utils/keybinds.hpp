#ifndef KEYBINDS
#define KEYBINDS

namespace ammonite {
  namespace utils {
    namespace controls {
      void setKeybind(unsigned short engineKey, int keycode);
      int getKeybind(unsigned short engineKey);
    }
  }
}

#endif

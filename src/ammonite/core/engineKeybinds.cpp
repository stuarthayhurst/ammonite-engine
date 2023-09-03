#include <map>

#include <GLFW/glfw3.h>

#include "../constants.hpp"

namespace ammonite {
  namespace input {
    namespace internal {
      namespace {
        std::map<AmmoniteEnum, int> keybindTracker = {
          {AMMONITE_EXIT, GLFW_KEY_ESCAPE}
        };
      }

      bool isEngineKeybindValid(AmmoniteEnum engineKeybind) {
        return keybindTracker.contains(engineKeybind);
      }

      int getExistingKeycode(AmmoniteEnum engineKeybind) {
        return keybindTracker[engineKeybind];
      }

      void setEngineKeybind(AmmoniteEnum engineKeybind, int keycode) {
        keybindTracker[engineKeybind] = keycode;
      }
    }
  }
}

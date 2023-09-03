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

      bool isKeybindInternal(int keycode) {
        //Check all engine keybinds for a match
        for (auto it = keybindTracker.begin(); it != keybindTracker.end(); it++) {
          if (it->second == keycode) {
            return true;
          }
        }

        return false;
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

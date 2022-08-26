#include <map>

#include <GLFW/glfw3.h>

#include "../constants.hpp"
#include "../internal/internalDebug.hpp"

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace {
        std::map<unsigned short, int> keybindTracker = {
          {AMMONITE_EXIT, GLFW_KEY_ESCAPE},
          {AMMONITE_FORWARD, GLFW_KEY_UP},
          {AMMONITE_BACK, GLFW_KEY_DOWN},
          {AMMONITE_UP, GLFW_KEY_SPACE},
          {AMMONITE_DOWN, GLFW_KEY_LEFT_SHIFT},
          {AMMONITE_LEFT, GLFW_KEY_LEFT},
          {AMMONITE_RIGHT, GLFW_KEY_RIGHT}
        };
      }

      namespace internal {
        std::map<unsigned short, int>* getKeybindTrackerPtr() {
          return &keybindTracker;
        }
      }

      void setKeybind(unsigned short engineKey, int keycode) {
        if (keybindTracker.contains(engineKey)) {
          keybindTracker[engineKey] = keycode;
        }
      }

      int getKeybind(unsigned short engineKey) {
        if (keybindTracker.contains(engineKey)) {
          return keybindTracker[engineKey];
        } else {
          return -1;
        }
      }
    }
  }
}

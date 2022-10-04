#ifndef INTERNALKEYBINDS
#define INTERNALKEYBINDS

#include <map>

#include "../constants.hpp"

/* Internally exposed header:
 - Allow access to keybind tracker internally
*/

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace internal {
        std::map<AmmoniteEnum, int>* getKeybindTrackerPtr();
      }
    }
  }
}

#endif

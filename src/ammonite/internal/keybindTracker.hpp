#ifndef INTERNALKEYBINDS
#define INTERNALKEYBINDS

/* Internally exposed header:
 - Allow access to keybind tracker internally
*/

#include <map>

#include "../constants.hpp"

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

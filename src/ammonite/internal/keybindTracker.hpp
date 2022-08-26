#ifndef INTERNALKEYBINDS
#define INTERNALKEYBINDS

#include <map>

/* Internally exposed header:
 - Allow access to keybind tracker internally
*/

namespace ammonite {
  namespace utils {
    namespace controls {
      namespace internal {
        std::map<unsigned short, int>* getKeybindTrackerPtr();
      }
    }
  }
}

#endif

#ifndef INTERNALINTERFACE
#define INTERNALINTERFACE

#include <map>

/* Internally exposed header:
 - Allow access to loading screen tracker internally
*/

namespace ammonite {
  namespace interface {
    struct LoadingScreen {
      float progress = 0.0f;
    };

    namespace internal {
      std::map<int, LoadingScreen>* getLoadingScreenTracker();
      int getActiveLoadingScreenId();
    }
  }
}

#endif

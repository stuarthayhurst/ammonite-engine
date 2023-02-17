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
      float width = 0.85f;
      float height = 0.04f;
      float heightOffset = 0.86f;
    };

    namespace internal {
      std::map<int, LoadingScreen>* getLoadingScreenTracker();
      int getActiveLoadingScreenId();
    }
  }
}

#endif

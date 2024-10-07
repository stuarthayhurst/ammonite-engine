#ifndef INTERNALINTERFACE
#define INTERNALINTERFACE

/* Internally exposed header:
 - Allow access to loading screen tracker internally
*/

#include <map>

#include <glm/glm.hpp>

#include "../types.hpp"

namespace ammonite {
  namespace interface {
    struct LoadingScreen {
      float progress = 0.0f;
      float width = 0.85f;
      float height = 0.04f;
      float heightOffset = 0.86f;
      glm::vec3 backgroundColour = glm::vec3(1.0f);
      glm::vec3 trackColour = glm::vec3(0.7f);
      glm::vec3 progressColour = glm::vec3(0.0f, 0.6f, 0.8f);
    };

    namespace internal {
      std::map<AmmoniteId, LoadingScreen>* getLoadingScreenTracker();
      AmmoniteId getActiveLoadingScreenId();
    }
  }
}

#endif

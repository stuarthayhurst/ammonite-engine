#ifndef INTERNALINTERFACE
#define INTERNALINTERFACE

#include <glm/glm.hpp>

#include "internal.hpp"
#include "types.hpp"

namespace ammonite {
  namespace interface {
    namespace AMMONITE_INTERNAL internal {
      struct LoadingScreen {
        float progress = 0.0f;
        float width = 0.85f;
        float height = 0.04f;
        float heightOffset = 0.86f;
        glm::vec3 backgroundColour = glm::vec3(1.0f);
        glm::vec3 trackColour = glm::vec3(0.7f);
        glm::vec3 progressColour = glm::vec3(0.0f, 0.6f, 0.8f);
      };

      LoadingScreen* getLoadingScreenPtr(AmmoniteId loadingScreenId);
    }

    //Exported by the engine
    AmmoniteId createLoadingScreen();
    void deleteLoadingScreen(AmmoniteId targetScreenId);
    void setActiveLoadingScreen(AmmoniteId targetScreenId);
    AmmoniteId getActiveLoadingScreenId();

    void setLoadingScreenProgress(AmmoniteId targetScreenId, float progress);
    void setLoadingScreenGeometry(AmmoniteId targetScreenId, float width,
                                  float height, float heightOffset);
    void setLoadingScreenColours(AmmoniteId targetScreenId, glm::vec3 backgroundColour,
                                 glm::vec3 trackColour, glm::vec3 progressColour);
  }
}

#endif

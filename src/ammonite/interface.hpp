#ifndef INTERFACE
#define INTERFACE

#include <glm/glm.hpp>

#include "types.hpp"

namespace ammonite {
  namespace interface {
    AmmoniteId createLoadingScreen();
    void deleteLoadingScreen(AmmoniteId targetScreenId);
    void setActiveLoadingScreen(AmmoniteId targetScreenId);
    AmmoniteId getActiveLoadingScreen();

    void setLoadingScreenProgress(AmmoniteId targetScreenId, float progress);
    void setLoadingScreenGeometry(AmmoniteId targetScreenId, float width,
                                  float height, float heightOffset);
    void setLoadingScreenColours(AmmoniteId targetScreenId, glm::vec3 backgroundColour,
                                 glm::vec3 trackColour, glm::vec3 progressColour);
  }
}

#endif

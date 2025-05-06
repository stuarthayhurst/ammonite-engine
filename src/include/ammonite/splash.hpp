#ifndef AMMONITESPLASH
#define AMMONITESPLASH

#include <glm/glm.hpp>

#include "utils/id.hpp"

namespace ammonite {
  namespace splash {
    AmmoniteId createSplashScreen();
    void deleteSplashScreen(AmmoniteId targetScreenId);
    void setActiveSplashScreen(AmmoniteId targetScreenId);
    AmmoniteId getActiveSplashScreenId();

    void setSplashScreenProgress(AmmoniteId targetScreenId, float progress);
    void setSplashScreenGeometry(AmmoniteId targetScreenId, float width,
                                  float height, float heightOffset);
    void setSplashScreenColours(AmmoniteId targetScreenId, glm::vec3 backgroundColour,
                                 glm::vec3 trackColour, glm::vec3 progressColour);
  }
}

#endif

#ifndef AMMONITESPLASH
#define AMMONITESPLASH

#include "maths/vectorTypes.hpp"
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
    void setSplashScreenColours(AmmoniteId targetScreenId,
                                const ammonite::Vec<float, 3>& backgroundColour,
                                const ammonite::Vec<float, 3>& trackColour,
                                const ammonite::Vec<float, 3>& progressColour);
  }
}

#endif

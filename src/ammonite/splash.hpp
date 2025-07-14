#ifndef INTERNALSPLASH
#define INTERNALSPLASH

#include "internal.hpp"
#include "maths/vector.hpp"
#include "utils/id.hpp"

//Include public interface
#include "../include/ammonite/splash.hpp" // IWYU pragma: export

namespace ammonite {
  namespace splash {
    namespace AMMONITE_INTERNAL internal {
      struct SplashScreen {
        float progress = 0.0f;
        float width = 0.85f;
        float height = 0.04f;
        float heightOffset = 0.86f;
        ammonite::Vec<float, 3> backgroundColour = {1.0f, 1.0f, 1.0f};
        ammonite::Vec<float, 3> trackColour = {0.7f, 0.7f, 0.7f};
        ammonite::Vec<float, 3> progressColour = {0.0f, 0.6f, 0.8f};
      };

      SplashScreen* getSplashScreenPtr(AmmoniteId splashScreenId);
    }
  }
}

#endif

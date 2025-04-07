#ifndef INTERNALSPLASH
#define INTERNALSPLASH

#include <glm/glm.hpp>

#include "internal.hpp"
#include "types.hpp"

//Include public interface
#include "../include/ammonite/splash.hpp" // IWYU pragma: export

namespace ammonite {
  namespace splash {
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
  }
}

#endif

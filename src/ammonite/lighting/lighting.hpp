#ifndef INTERNALLIGHTING
#define INTERNALLIGHTING

extern "C" {
  #include <epoxy/gl.h>
}

#include "../maths/vector.hpp"
#include "../utils/id.hpp"
#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/lighting/lighting.hpp" // IWYU pragma: export

namespace ammonite {
  namespace lighting {
    namespace AMMONITE_INTERNAL internal {
      struct LightSource {
        ammonite::Vec<float, 3> geometry = {0.0f, 0.0f, 0.0f};
        ammonite::Vec<float, 3> diffuse = {1.0f, 1.0f, 1.0f};
        ammonite::Vec<float, 3> specular = {0.3f, 0.3f, 0.3f};
        float power = 1.0f;
        AmmoniteId lightId = 0;
        AmmoniteId modelId = 0;
        unsigned int lightIndex;
      };

      void unlinkByModel(AmmoniteId modelId);

      void startUpdateLightSources();
      void finishUpdateLightSources();
      void setLightSourcesChanged();
      void destroyLightSystem();
    }
  }
}

#endif

#ifndef INTERNALLIGHTING
#define INTERNALLIGHTING

#include <map>
#include <glm/glm.hpp>

#include "../internal.hpp"
#include "../types.hpp"

//Include public interface
#include "../../include/ammonite/lighting/lighting.hpp" // IWYU pragma: export

namespace ammonite {
  namespace lighting {
    namespace AMMONITE_INTERNAL internal {
      struct LightSource {
        glm::vec3 geometry = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specular = glm::vec3(0.3f, 0.3f, 0.3f);
        float power = 1.0f;
        AmmoniteId lightId = 0;
        AmmoniteId modelId = 0;
        unsigned int lightIndex;
      };

      void unlinkByModel(AmmoniteId modelId);
      LightSource* getLightSourcePtr(AmmoniteId lightId);

      void updateLightSources();
      void setLightSourcesChanged();
      void destroyLightSystem();

      std::map<AmmoniteId, LightSource>* getLightTrackerPtr();
      glm::mat4** getLightTransformsPtr();
    }
  }
}

#endif

#ifndef LIGHTTRACKER
#define LIGHTTRACKER

/* Internally exposed header:
 - Allow access to light tracker internally
 - Allow access to light transforms internally
 - Allow triggering light sources to be processed for the GPU
*/

#include <map>
#include <glm/glm.hpp>

#include "lightTypes.hpp"
#include "../../types.hpp"

namespace ammonite {
  namespace lighting {
    namespace internal {
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

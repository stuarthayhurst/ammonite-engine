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

namespace ammonite {
  namespace lighting {
    namespace internal {
      void unlinkByModel(int modelId);
      LightSource* getLightSourcePtr(int lightId);

      void updateLightSources();
      void setLightSourcesChanged();

      std::map<int, LightSource>* getLightTrackerPtr();
      glm::mat4** getLightTransformsPtr();
    }
  }
}

#endif

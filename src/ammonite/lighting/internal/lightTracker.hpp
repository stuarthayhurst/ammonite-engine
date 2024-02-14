#ifndef LIGHTTRACKER
#define LIGHTTRACKER

/* Internally exposed header:
 - Define data structures for lights
*/

#include <map>
#include <glm/glm.hpp>

#include "lightTypes.hpp"

namespace ammonite {
  namespace lighting {
    namespace internal {
      void unlinkByModel(int modelId);
      LightSource* getLightSourcePtr(int lightId);

      std::map<int, LightSource>* getLightTrackerPtr();
      glm::mat4** getLightTransformsPtr();
    }
  }
}

#endif

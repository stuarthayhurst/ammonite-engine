#ifndef LIGHTTYPES
#define LIGHTTYPES

/* Internally exposed header:
 - Define data structures for lights
*/

#include <glm/glm.hpp>

#include "../../types.hpp"

namespace ammonite {
  namespace lighting {
    namespace internal {
      struct LightSource {
        glm::vec3 geometry = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specular = glm::vec3(0.3f, 0.3f, 0.3f);
        float power = 1.0f;
        AmmoniteId lightId = 0;
        AmmoniteId modelId = 0;
        unsigned int lightIndex;
      };
    }
  }
}

#endif

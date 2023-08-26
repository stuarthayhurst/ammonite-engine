#ifndef LIGHTTYPES
#define LIGHTTYPES

/* Internally exposed header:
 - Allow access to light tracker internally
 - Allow access to light transforms internally
*/

#include <glm/glm.hpp>

namespace ammonite {
  namespace lighting {
    namespace internal {
      struct LightSource {
        glm::vec3 geometry = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specular = glm::vec3(0.3f, 0.3f, 0.3f);
        float power = 1.0f;
        int lightId;
        int modelId = -1;
      };
    }
  }
}

#endif

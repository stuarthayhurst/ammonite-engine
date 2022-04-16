#ifndef LIGHTING
#define LIGHTING

#include <glm/glm.hpp>

namespace ammonite {
  namespace lighting {
    void setAmbientLight(glm::vec3 newAmbientLight);
    glm::vec3 getAmbientLight();
  }
}

#endif

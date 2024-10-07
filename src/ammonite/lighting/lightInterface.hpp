#ifndef LIGHTINTERFACE
#define LIGHTINTERFACE

#include <glm/glm.hpp>

#include "../types.hpp"

namespace ammonite {
  namespace lighting {
    glm::vec3 getAmbientLight();
    void setAmbientLight(glm::vec3 newAmbientLight);

    namespace properties {
      glm::vec3 getGeometry(AmmoniteId lightId);
      glm::vec3 getColour(AmmoniteId lightId);
      float getPower(AmmoniteId lightId);

      void setGeometry(AmmoniteId lightId, glm::vec3 geometry);
      void setColour(AmmoniteId lightId, glm::vec3 colour);
      void setPower(AmmoniteId lightId, float power);
    }
  }
}

#endif

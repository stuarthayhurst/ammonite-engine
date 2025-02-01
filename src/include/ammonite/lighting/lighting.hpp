#ifndef LIGHTING
#define LIGHTING

#include <map>
#include <glm/glm.hpp>

#include "../types.hpp"

namespace ammonite {
  namespace lighting {
    namespace properties {
      glm::vec3 getGeometry(AmmoniteId lightId);
      glm::vec3 getColour(AmmoniteId lightId);
      float getPower(AmmoniteId lightId);

      void setGeometry(AmmoniteId lightId, glm::vec3 geometry);
      void setColour(AmmoniteId lightId, glm::vec3 colour);
      void setPower(AmmoniteId lightId, float power);
    }

    glm::vec3 getAmbientLight();
    void setAmbientLight(glm::vec3 newAmbientLight);

    unsigned int getMaxLightCount();
    AmmoniteId createLightSource();
    void deleteLightSource(AmmoniteId lightId);

    void linkModel(AmmoniteId lightId, AmmoniteId modelId);
    void unlinkModel(AmmoniteId lightId);
  }
}

#endif

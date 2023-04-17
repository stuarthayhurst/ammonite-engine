#ifndef LIGHTINTERFACE
#define LIGHTINTERFACE

#include <glm/glm.hpp>

namespace ammonite {
  namespace lighting {
//Fits in both storage and interface, but can only be defined once
#ifndef UPDATESOURCESCALL
#define UPDATESOURCESCALL
    void updateLightSources();
#endif

    namespace properties {
      glm::vec3 getGeometry(int lightId);
      glm::vec3 getColour(int lightId);
      float getPower(int lightId);

      void setGeometry(int lightId, glm::vec3 geometry);
      void setColour(int lightId, glm::vec3 colour);
      void setPower(int lightId, float power);
    }
  }
}

#endif

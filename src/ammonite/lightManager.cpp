#include <glm/glm.hpp>

namespace ammonite {
  namespace lighting {
    //
  }

  namespace {
    glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);
  }

  //Exposed light handling methods
  namespace lighting {
    void setAmbientLight(glm::vec3 newAmbientLight) {
      ambientLight = newAmbientLight;
    }

    glm::vec3 getAmbientLight() {
      return ambientLight;
    }
  }
}

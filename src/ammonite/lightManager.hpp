#ifndef LIGHTING
#define LIGHTING

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace ammonite {
  namespace lighting {
    int getMaxLightCount();
    int createLightSource();
    void deleteLightSource(int lightId);
    void updateLightSources();

    void linkModel(int lightId, int modelId);
    void unlinkModel(int lightId);

    glm::vec3 getAmbientLight();
    void setAmbientLight(glm::vec3 newAmbientLight);

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

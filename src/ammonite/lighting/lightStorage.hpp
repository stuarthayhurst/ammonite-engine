#ifndef LIGHTSTORAGE
#define LIGHTSTORAGE

#include <glm/glm.hpp>

namespace ammonite {
  namespace lighting {
    int getMaxLightCount();
    int createLightSource();
    void deleteLightSource(int lightId);

    void linkModel(int lightId, int modelId);
    void unlinkModel(int lightId);

    glm::vec3 getAmbientLight();
    void setAmbientLight(glm::vec3 newAmbientLight);

//Fits in both storage and interface, but can only be defined once
#ifndef UPDATESOURCESCALL
#define UPDATESOURCESCALL
    void updateLightSources();
#endif
  }
}

#endif

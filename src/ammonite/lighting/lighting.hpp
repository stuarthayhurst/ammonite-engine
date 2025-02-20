#ifndef INTERNALLIGHTING
#define INTERNALLIGHTING

#include <map>
#include <glm/glm.hpp>

#include "../internal.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace lighting {
    namespace AMMONITE_INTERNAL internal {
      struct LightSource {
        glm::vec3 geometry = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specular = glm::vec3(0.3f, 0.3f, 0.3f);
        float power = 1.0f;
        AmmoniteId lightId = 0;
        AmmoniteId modelId = 0;
        unsigned int lightIndex;
      };

      void unlinkByModel(AmmoniteId modelId);
      LightSource* getLightSourcePtr(AmmoniteId lightId);

      void updateLightSources();
      void setLightSourcesChanged();
      void destroyLightSystem();

      std::map<AmmoniteId, LightSource>* getLightTrackerPtr();
      glm::mat4** getLightTransformsPtr();
    }

    //Exported by the engine
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

#ifndef MODELINTERFACE
#define MODELINTERFACE

#include <glm/glm.hpp>

#include "../types.hpp"

namespace ammonite {
  namespace models {
    namespace position {
      glm::vec3 getPosition(AmmoniteId modelId);
      glm::vec3 getScale(AmmoniteId modelId);
      glm::vec3 getRotation(AmmoniteId modelId);

      //Absolute movements
      void setPosition(AmmoniteId modelId, glm::vec3 position);
      void setScale(AmmoniteId modelId, glm::vec3 scale);
      void setScale(AmmoniteId modelId, float scaleMultiplier);
      void setRotation(AmmoniteId modelId, glm::vec3 rotation);

      //Relative adjustments
      void translateModel(AmmoniteId modelId, glm::vec3 translation);
      void scaleModel(AmmoniteId modelId, glm::vec3 scale);
      void scaleModel(AmmoniteId modelId, float scaleMultiplier);
      void rotateModel(AmmoniteId modelId, glm::vec3 rotation);
    }
  }
}

#endif

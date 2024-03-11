#ifndef MODELINTERFACE
#define MODELINTERFACE

#include <glm/glm.hpp>

#include "../enums.hpp"

namespace ammonite {
  namespace models {
    namespace draw {
      void setDrawMode(int modelId, AmmoniteEnum drawMode);
    }

    namespace position {
      glm::vec3 getPosition(int modelId);
      glm::vec3 getScale(int modelId);
      glm::vec3 getRotation(int modelId);

      //Absolute movements
      void setPosition(int modelId, glm::vec3 position);
      void setScale(int modelId, glm::vec3 scale);
      void setScale(int modelId, float scaleMultiplier);
      void setRotation(int modelId, glm::vec3 rotation);

      //Relative adjustments
      void translateModel(int modelId, glm::vec3 translation);
      void scaleModel(int modelId, glm::vec3 scale);
      void scaleModel(int modelId, float scaleMultiplier);
      void rotateModel(int modelId, glm::vec3 rotation);
    }
  }
}

#endif

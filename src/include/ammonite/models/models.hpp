#ifndef MODELS
#define MODELS

#include <string>

#include <glm/glm.hpp>

#include "../enums.hpp"
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

    AmmoniteId createModel(std::string objectPath);
    AmmoniteId createModel(std::string objectPath, bool flipTexCoords, bool srgbTextures);
    void deleteModel(AmmoniteId modelId);
    AmmoniteId copyModel(AmmoniteId modelId);

    bool applyTexture(AmmoniteId modelId, AmmoniteEnum textureType, std::string texturePath);
    bool applyTexture(AmmoniteId modelId, AmmoniteEnum textureType, std::string texturePath,
                      bool srgbTexture);
    unsigned int getIndexCount(AmmoniteId modelId);
    unsigned int getVertexCount(AmmoniteId modelId);
    void setDrawMode(AmmoniteId modelId, AmmoniteEnum drawMode);
  }
}

#endif

#ifndef MODELSTORAGE
#define MODELSTORAGE

#include "../enums.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace models {
    AmmoniteId createModel(std::string objectPath, bool* externalSuccess);
    AmmoniteId createModel(std::string objectPath, bool flipTexCoords, bool srgbTextures,
                           bool* externalSuccess);
    void deleteModel(AmmoniteId modelId);
    AmmoniteId copyModel(AmmoniteId modelId);

    void applyTexture(AmmoniteId modelId, AmmoniteEnum textureType, std::string texturePath,
                      bool* externalSuccess);
    void applyTexture(AmmoniteId modelId, AmmoniteEnum textureType, std::string texturePath,
                      bool srgbTexture, bool* externalSuccess);
    unsigned int getIndexCount(AmmoniteId modelId);
    unsigned int getVertexCount(AmmoniteId modelId);
    void setDrawMode(AmmoniteId modelId, AmmoniteEnum drawMode);
  }
}

#endif

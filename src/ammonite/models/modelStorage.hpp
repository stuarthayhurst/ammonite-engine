#ifndef MODELSTORAGE
#define MODELSTORAGE

#include <string>

#include "../enums.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace models {
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

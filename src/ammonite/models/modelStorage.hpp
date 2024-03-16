#ifndef MODELSTORAGE
#define MODELSTORAGE

#include "../enums.hpp"

namespace ammonite {
  namespace models {
    int createModel(const char* objectPath, bool* externalSuccess);
    int createModel(const char* objectPath, bool flipTexCoords, bool srgbTextures, bool* externalSuccess);
    void deleteModel(int modelId);
    int copyModel(int modelId);

    void applyTexture(int modelId, AmmoniteEnum textureType, const char* texturePath,
                      bool* externalSuccess);
    void applyTexture(int modelId, AmmoniteEnum textureType, const char* texturePath,
                      bool srgbTexture, bool* externalSuccess);
    int getVertexCount(int modelId);
    void setDrawMode(int modelId, AmmoniteEnum drawMode);
  }
}

#endif

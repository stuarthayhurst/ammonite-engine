#ifndef MODELSTORAGE
#define MODELSTORAGE

#include "../constants.hpp"

namespace ammonite {
  namespace models {
    int createModel(const char* objectPath, bool* externalSuccess);
    int createModel(const char* objectPath, bool flipTexCoords, bool srgbTextures, bool* externalSuccess);
    void deleteModel(int modelId);
    int copyModel(int modelId);

    void unloadModel(int modelId);
    void reloadModel(int modelId);

    void applyTexture(int modelId, AmmoniteEnum textureType, const char* texturePath, bool* externalSuccess);
    void applyTexture(int modelId, AmmoniteEnum textureType, const char* texturePath, bool srgbTexture,
                      bool* externalSuccess);
    int getVertexCount(int modelId);
  }
}

#endif

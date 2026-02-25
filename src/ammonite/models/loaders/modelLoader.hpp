#ifndef INTERNALMODELLOADER
#define INTERNALMODELLOADER

#include <string>
#include <vector>

#include "../modelTypes.hpp"
#include "../../visibility.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace models {
    namespace internal {
      //Model loaders
      bool loadFileObject(ModelData* modelData,
                          std::vector<RawMeshData>* rawMeshDataVec,
                          const ModelLoadInfo& modelLoadInfo);
      bool loadMemoryObject(ModelData* modelData,
                            std::vector<RawMeshData>* rawMeshDataVec,
                            const ModelLoadInfo& modelLoadInfo);

      //Texture loaders
      GLuint queueTextureLoad(const std::string& texturePath, bool flipTexture,
                              bool srgbTexture);
      bool uploadQueuedTextures();
    }
  }
}

#endif

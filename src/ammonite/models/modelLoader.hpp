#ifndef INTERNALMODELLOADER
#define INTERNALMODELLOADER

#include <string>
#include <vector>

#include "modelTypes.hpp"

#include "../visibility.hpp"

namespace ammonite {
  namespace models {
    namespace AMMONITE_INTERNAL internal {
      //Model loaders
      bool loadObject(const std::string& objectPath, ModelData* modelData,
                      std::vector<RawMeshData>* rawMeshDataVec,
                      const ModelLoadInfo& modelLoadInfo);
    }
  }
}

#endif

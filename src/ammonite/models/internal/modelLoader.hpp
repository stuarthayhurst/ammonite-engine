#ifndef MODELLOADER
#define MODELLOADER

/* Internally exposed header:
 - Provide model loader and ModelLoadInfo
*/

#include <string>
#include "modelTypes.hpp"

struct ModelLoadInfo {
  std::string modelDirectory;
  bool flipTexCoords;
  bool srgbTextures;
};

namespace ammonite {
  namespace models {
    namespace internal {
      bool loadObject(std::string objectPath, models::internal::ModelData* modelObjectData,
                      ModelLoadInfo modelLoadInfo);
    }
  }
}

#endif

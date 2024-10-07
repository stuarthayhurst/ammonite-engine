#ifndef SKYBOX
#define SKYBOX

#include "types.hpp"

namespace ammonite {
  namespace skybox {
    AmmoniteId getActiveSkybox();
    void setActiveSkybox(AmmoniteId skyboxId);

    AmmoniteId createSkybox(std::string texturePaths[6], bool* externalSuccess);
    AmmoniteId createSkybox(std::string texturePaths[6], bool flipTextures,
                            bool srgbTextures, bool* externalSuccess);
    AmmoniteId loadDirectory(std::string directoryPath, bool* externalSuccess);
    AmmoniteId loadDirectory(std::string directoryPath, bool flipTextures,
                             bool srgbTextures, bool* externalSuccess);

    void deleteSkybox(AmmoniteId skyboxId);
  }
}

#endif

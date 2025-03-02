#ifndef SKYBOX
#define SKYBOX

#include <string>

#include "types.hpp"

namespace ammonite {
  namespace skybox {
    AmmoniteId getActiveSkybox();
    void setActiveSkybox(AmmoniteId skyboxId);

    AmmoniteId createSkybox(std::string texturePaths[6]);
    AmmoniteId createSkybox(std::string texturePaths[6], bool flipTextures, bool srgbTextures);
    AmmoniteId loadDirectory(const std::string& directoryPath);
    AmmoniteId loadDirectory(const std::string& directoryPath, bool flipTextures, bool srgbTextures);

    void deleteSkybox(AmmoniteId skyboxId);
  }
}

#endif

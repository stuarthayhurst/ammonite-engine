#ifndef INTERNALSKYBOX
#define INTERNALSKYBOX

#include <string>

#include "types.hpp"

namespace ammonite {
  namespace skybox {
    AmmoniteId getActiveSkybox();
    void setActiveSkybox(AmmoniteId skyboxId);

    AmmoniteId createSkybox(std::string texturePaths[6]);
    AmmoniteId createSkybox(std::string texturePaths[6], bool flipTextures, bool srgbTextures);
    AmmoniteId loadDirectory(std::string directoryPath);
    AmmoniteId loadDirectory(std::string directoryPath, bool flipTextures, bool srgbTextures);

    void deleteSkybox(AmmoniteId skyboxId);
  }
}

#endif

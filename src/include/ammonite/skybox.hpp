#ifndef AMMONITESKYBOX
#define AMMONITESKYBOX

#include <string>

#include "utils/id.hpp"
#include "visibility.hpp"

static constexpr bool ASSUME_FLIP_SKYBOX_FACES = false;

namespace AMMONITE_EXPOSED ammonite {
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

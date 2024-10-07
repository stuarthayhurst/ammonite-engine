#ifndef LIGHTSTORAGE
#define LIGHTSTORAGE

#include <glm/glm.hpp>

#include "../types.hpp"

namespace ammonite {
  namespace lighting {
    unsigned int getMaxLightCount();
    AmmoniteId createLightSource();
    void deleteLightSource(AmmoniteId lightId);

    void linkModel(AmmoniteId lightId, AmmoniteId modelId);
    void unlinkModel(AmmoniteId lightId);
  }
}

#endif

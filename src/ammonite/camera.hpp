#ifndef CAMERA
#define CAMERA

#include <glm/glm.hpp>

#include "types.hpp"

namespace ammonite {
  namespace camera {
    AmmoniteId createCamera();
    void deleteCamera(AmmoniteId cameraId);

    AmmoniteId getActiveCamera();
    void setActiveCamera(AmmoniteId cameraId);

    glm::vec3 getPosition(AmmoniteId cameraId);
    glm::vec3 getDirection(AmmoniteId cameraId);
    float getHorizontal(AmmoniteId cameraId);
    float getVertical(AmmoniteId cameraId);
    float getFieldOfView(AmmoniteId cameraId);

    void setPosition(AmmoniteId cameraId, glm::vec3 newPosition);
    void setHorizontal(AmmoniteId cameraId, float newHorizontal);
    void setVertical(AmmoniteId cameraId, float newVertical);
    void setFieldOfView(AmmoniteId cameraId, float newFov);
  }
}

#endif

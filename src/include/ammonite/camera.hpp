#ifndef AMMONITECAMERA
#define AMMONITECAMERA

#include <glm/glm.hpp>

#include "utils/id.hpp"

namespace ammonite {
  namespace camera {
    AmmoniteId createCamera();
    void deleteCamera(AmmoniteId cameraId);

    AmmoniteId getActiveCamera();
    void setActiveCamera(AmmoniteId cameraId);

    glm::vec3 getPosition(AmmoniteId cameraId);
    glm::vec3 getDirection(AmmoniteId cameraId);
    double getHorizontal(AmmoniteId cameraId);
    double getVertical(AmmoniteId cameraId);
    float getFieldOfView(AmmoniteId cameraId);

    void setPosition(AmmoniteId cameraId, glm::vec3 position);
    void setDirection(AmmoniteId cameraId, glm::vec3 direction);
    void setAngle(AmmoniteId cameraId, double horizontal, double vertical);
    void setFieldOfView(AmmoniteId cameraId, float fov);
  }
}

#endif

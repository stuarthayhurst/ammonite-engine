#ifndef AMMONITECAMERA
#define AMMONITECAMERA

#include "maths/vectorTypes.hpp"
#include "utils/id.hpp"

namespace ammonite {
  namespace camera {
    AmmoniteId createCamera();
    void deleteCamera(AmmoniteId cameraId);

    AmmoniteId getActiveCamera();
    void setActiveCamera(AmmoniteId cameraId);

    void getPosition(AmmoniteId cameraId, ammonite::Vec<float, 3>& position);
    void getDirection(AmmoniteId cameraId, ammonite::Vec<float, 3>& direction);
    double getHorizontal(AmmoniteId cameraId);
    double getVertical(AmmoniteId cameraId);
    float getFieldOfView(AmmoniteId cameraId);

    void setPosition(AmmoniteId cameraId, const ammonite::Vec<float, 3>& position);
    void setDirection(AmmoniteId cameraId, const ammonite::Vec<float, 3>& direction);
    void setAngle(AmmoniteId cameraId, double horizontal, double vertical);
    void setFieldOfView(AmmoniteId cameraId, float fov);
  }
}

#endif

#ifndef CAMERA
#define CAMERA

#include <glm/glm.hpp>

namespace ammonite {
  namespace camera {
    int createCamera();
    void deleteCamera(int cameraId);

    int getActiveCamera();
    void setActiveCamera(int cameraId);

    glm::vec3 getPosition(int cameraId);
    glm::vec3 getDirection(int cameraId);
    float getHorizontal(int cameraId);
    float getVertical(int cameraId);
    float getFieldOfView(int cameraId);

    void setPosition(int cameraId, glm::vec3 newPosition);
    void setHorizontal(int cameraId, float newHorizontal);
    void setVertical(int cameraId, float newVertical);
    void setFieldOfView(int cameraId, float newFov);
  }
}

#endif

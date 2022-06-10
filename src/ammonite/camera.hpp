#ifndef CAMERA
#define CAMERA

#include <glm/glm.hpp>

namespace ammonite {
  namespace camera {
    glm::vec3 getPosition();
    float getHorizontal();
    float getVertical();
    float getFieldOfView();

    void setPosition(glm::vec3 newPosition);
    void setHorizontal(float newHorizontal);
    void setVertical(float newVertical);
    void setFieldOfView(float newFov);
  }
}

#endif
